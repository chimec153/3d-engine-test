#include "shared.hlsl"

// Editor selection outline — two-pass:
//   1) MaskVS / MaskPS — render the selected mesh container's geometry into a
//      single-channel RT (white where the container's pixels land).
//   2) OutlineFullScreenVS / OutlineCompositePS — full-screen pass that
//      reads the mask and writes outline color only on mask edges. Run after
//      PostProcessing on the back buffer with alpha-blend.

struct VSMaskIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float3 blendWeight : BLENDWEIGHT;
    float2 uv : Texcoord;
    float4 blendIndex : BLENDINDICES;
};

float4 MaskVS(VSMaskIn input) : SV_Position
{
    return mul(float4(input.pos, 1.f), g_matTransform);
}

float4 MaskPS() : SV_Target
{
    return float4(1, 1, 1, 1);
}

struct VS_OUTLINE_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

static const float2 arrOutlineBasePos[4] =
{
    float2(-1.0, 1.0),
    float2(1.0, 1.0),
    float2(-1.0, -1.0),
    float2(1.0, -1.0),
};

static const float2 arrOutlineUV[4] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
};

VS_OUTLINE_OUT OutlineFullScreenVS(uint VertexID : SV_VertexID)
{
    VS_OUTLINE_OUT output;
    output.pos = float4(arrOutlineBasePos[VertexID].xy, 0.0, 1.0);
    output.uv = arrOutlineUV[VertexID].xy;
    return output;
}

Texture2D<float4> g_OutlineMaskTex : register(t0);

cbuffer OutlineCB : register(b0)
{
    float4 g_vOutlineColor;
    float2 g_vOutlineTexelSize;
    int g_iOutlineThickness;
    float g_fOutlinePadding;
}

float4 OutlineCompositePS(VS_OUTLINE_OUT input) : SV_Target
{
    float c = g_OutlineMaskTex.Sample(g_sPoint, input.uv).r;

    float2 dx = float2(g_vOutlineTexelSize.x * g_iOutlineThickness, 0);
    float2 dy = float2(0, g_vOutlineTexelSize.y * g_iOutlineThickness);

    float n = g_OutlineMaskTex.Sample(g_sPoint, input.uv + dy).r;
    float s = g_OutlineMaskTex.Sample(g_sPoint, input.uv - dy).r;
    float e = g_OutlineMaskTex.Sample(g_sPoint, input.uv + dx).r;
    float w = g_OutlineMaskTex.Sample(g_sPoint, input.uv - dx).r;

    float edge = max(max(abs(n - c), abs(s - c)), max(abs(e - c), abs(w - c)));

    if (edge < 0.5)
        discard;

    return g_vOutlineColor;
}

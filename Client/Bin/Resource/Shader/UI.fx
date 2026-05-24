#include "shared.hlsl"

const static float4 g_vUIPosition[4] =
{
    {0.f,1.f,0.f,1.f},
    {1.f,1.f,0.f,1.f},
    {0.f,0.f,0.f,1.f},
    {1.f,0.f,0.f,1.f},
};
const static float2 g_vUIUV[4] =
{
    { 0.f, 0.f},
    { 1.f, 0.f},
    { 0.f, 1.f},
    { 1.f, 1.f},
};

// Standalone tint cbuffer for the tinted UI pixel shader. Slots
// b0-b12 are taken by shared.hlsl cbuffers (PaperBurn lives at b10,
// Fog at b12). b13 is the first free slot — picking it avoids the
// "constant buffer too small" warning that bound a 16-byte UITint
// over the 80-byte PaperBurn slot.
cbuffer UITint : register(b13)
{
    float4 g_vUITint;   // (r, g, b, alpha_master)
}

VSMultiOut VS_UI(uint iVertexID :   SV_VertexID)
{
    VSMultiOut output = (VSMultiOut)0;

    output.pos = mul(g_vUIPosition[iVertexID], g_matTransform);
    // UV sub-region remap — caller pushes the b5 UI cbuffer per-draw
    // (EnemyCountHUD picks a digit cell; Text pushes the full-quad
    // (0,0)-(1,1) so its glyph atlas samples end-to-end). The earlier
    // Phase-E5 cleanup orphaned this push for UIControl-style callers,
    // which collapsed every UV to (0,0). Callers that don't need a
    // sub-region must still push (0,0)/(1,1) so a previous draw's
    // sub-region doesn't leak.
    output.uv = g_vUIUV[iVertexID] * (g_vUIEndUV - g_vUIStartUV) + g_vUIStartUV;

    return output;
}

float4 PS_UI(VSMultiOut input)  :   SV_TARGET
{
    return g_Texture.Sample(g_sAnisotropic, input.uv);
}

// Glyph-atlas friendly path: samples *alpha only* from the texture and
// modulates by g_vUITint. Used by floating combat text — atlas RGB is
// arbitrary, the alpha channel carries the glyph shape and the tint
// chooses the final colour + master opacity.
float4 PS_UITint(VSMultiOut input) : SV_TARGET
{
    float fGlyphA = g_Texture.Sample(g_sAnisotropic, input.uv).a;
    return float4(g_vUITint.rgb, g_vUITint.a * fGlyphA);
}

// ---- Instanced glyph path -----------------------------------------------
// Per-instance data lives in a StructuredBuffer at register t1. The VS
// reads the entry indexed by SV_InstanceID, builds a unit quad off
// SV_VertexID, and emits both UV and tint to the PS — letting the host
// draw N glyphs (numbers, outlines, all targets, all frames) with a
// single Draw call.

struct GlyphInstance
{
    float4 ndcRect;   // (x, y, w, h) in NDC — bottom-left + size
    float4 uvRect;    // (u0, v0, u1, v1) — atlas top-left + bottom-right
    float4 tint;      // (r, g, b, alpha_master)
};

StructuredBuffer<GlyphInstance> g_Glyphs : register(t1);

struct UIInstOut
{
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD0;
    float4 tint : COLOR0;
};

UIInstOut VS_UIInst(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    GlyphInstance g = g_Glyphs[iid];

    // Vertex layout — matches g_vUIPosition / g_vUIUV from the static
    // UI VS, derived without a VB so we never bind one:
    //   vid 0 → quad top-left,     uv top-left
    //   vid 1 → quad top-right,    uv top-right
    //   vid 2 → quad bottom-left,  uv bottom-left
    //   vid 3 → quad bottom-right, uv bottom-right
    float2 u01 = float2((vid & 1) ? 1.f : 0.f,
                        1.f - float((vid >> 1) & 1));

    UIInstOut o;
    o.pos = float4(
        g.ndcRect.x + u01.x * g.ndcRect.z,
        g.ndcRect.y + u01.y * g.ndcRect.w,
        0.f, 1.f);
    o.uv = float2(
        lerp(g.uvRect.x, g.uvRect.z, u01.x),
        lerp(g.uvRect.y, g.uvRect.w, 1.f - u01.y));
    o.tint = g.tint;
    return o;
}

float4 PS_UIInst(UIInstOut input) : SV_TARGET
{
    float fA = g_Texture.Sample(g_sAnisotropic, input.uv).a;
    return float4(input.tint.rgb, input.tint.a * fA);
}

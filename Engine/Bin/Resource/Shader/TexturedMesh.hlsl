// RenderV2 textured cube shader. Single MVP CB at b0, single texture at t0
// + linear wrap sampler at s0.
cbuffer PerObject : register(b0)
{
    float4x4 mvp;
};

Texture2D    diffuse : register(t0);
SamplerState samp    : register(s0);

struct VsIn
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VsOut
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

VsOut VSMain(VsIn input)
{
    VsOut o;
    o.pos = mul(float4(input.pos, 1.0), mvp);
    o.uv  = input.uv;
    return o;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    return diffuse.Sample(samp, input.uv);
}

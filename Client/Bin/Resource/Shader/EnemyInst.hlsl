// EnemyInst.hlsl — game-specific enemy shaders.
//
// Lives in the GAME resource tree (not the engine) so enemy-only rendering
// concerns (per-instance hit flash + per-instance dissolve) stay out of the
// shared engine shaders. #include "shared.hlsl" reuses the engine's GBuffer
// layout (PSOut), transform/material cbuffers, and helpers (EncodeShadingId).
//
// Four entry points wired by RenderManager's naming convention:
//   EnemyVS / EnemyPS         — solo path (bucket size < 2 or no Inst variant)
//   EnemyVSInst / EnemyPSInst — DrawInstanced fast path
//
// The instanced path carries hit-flash colour and dissolve time as
// per-instance vertex-stream attributes (HitFlash, PaperTime), packed by
// Client::EnemyMeshRenderer::GetInstData. That lets many same-kind enemies
// collapse into one DrawInstanced call while each still flashes / dissolves
// independently — the two effects that previously forced per-enemy solo draws.

#include "shared.hlsl"

// ---------------------------------------------------------------------------
// Solo path. Matches the engine "Standard" input layout + VS_NoSkin /
// PS_NoDiffuseNoSpecMapNoNormalMap so a lone enemy (no same-kind neighbour to
// batch with) still renders correctly. Dissolve is intentionally omitted here
// — see EnemyPSInst for the per-instance dissolve. Hit flash still works via
// the material cbuffer (ApplyHitFlash reads g_vHitFlash).
// ---------------------------------------------------------------------------
VSOut EnemyVS(VSStandardIn input)
{
    VSOut output;

    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.view = mul(input.pos, (float3x3) g_matWorldView);
    output.uv = input.uv;
    output.normal = normalize(mul(input.normal, (float3x3) g_matWorldView));
    output.tangent.xyz = normalize(mul(input.tangent.xyz, (float3x3) g_matWorldView));
    output.tangent.w = input.tangent.w;
    output.clip = output.pos;

    return output;
}

PSOut EnemyPS(VSOut input)
{
    PSOut output;

    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    output.value0.xyz = g_vDiffuseColor.xyz;
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = 0.f;
    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

// ---------------------------------------------------------------------------
// Instanced path. Game-specific vertex struct: the per-vertex block mirrors
// the engine "Standard" layout (slot 0), the per-instance block (slot 1) is
// transform (World/View/LIGHTVP) + material (Material0..3) — same offsets the
// engine's GetInstData already writes — followed by the two game-only
// attributes HitFlash + PaperTime that EnemyMeshRenderer appends.
// ---------------------------------------------------------------------------
struct VSEnemyInstIn
{
    // per-vertex (slot 0) — must match the enemy mesh / "Standard" layout
    float4 tangent     : Tangent;
    float4 blendIndex  : BLENDINDICES;
    float3 pos         : Position;
    float3 normal      : Normal;
    float3 blendWeight : BLENDWEIGHT;
    float2 uv          : Texcoord;
    // per-instance (slot 1)
    matrix WVP         : World;        // byte   0
    matrix WV          : View;         // byte  64
    matrix lightWVP    : LIGHTVP;      // byte 128
    float4 diffuse     : Material0;    // byte 192
    float4 specular    : Material1;    // byte 208
    float2 roughness   : Material2;    // byte 224
    float  fraction    : Material3;    // byte 232
    float4 hitFlash    : HitFlash;     // byte 236  (game-only)
    float  paperTime   : PaperTime;    // byte 252  (game-only)
};

struct VSEnemyInstOut
{
    float4 pos          : SV_Position;
    float2 uv           : Texcoord;
    float3 normal       : NORMAL;
    float4 tangent      : TANGENT;
    float4 vDiffuseColor : Diffuse;
    float4 vSpecularColor : Specular;
    float2 vMaterialRoughness : Material;
    float  fMaterialFraction : MaterialFrac;
    float4 hitFlash     : HitFlash;
    float  paperTime    : PaperTime;
    float4 clip         : Position;
};

VSEnemyInstOut EnemyVSInst(VSEnemyInstIn input)
{
    VSEnemyInstOut output;

    output.pos = mul(float4(input.pos, 1.f), input.WVP);
    output.uv = input.uv;
    output.normal = normalize(mul(input.normal, (float3x3) input.WV));
    output.tangent.xyz = normalize(mul(input.tangent.xyz, (float3x3) input.WV));
    output.tangent.w = input.tangent.w;

    output.vDiffuseColor = input.diffuse;
    output.vSpecularColor = input.specular;
    output.vMaterialRoughness = input.roughness;
    output.fMaterialFraction = input.fraction;

    output.hitFlash = input.hitFlash;
    output.paperTime = input.paperTime;
    output.clip = output.pos;

    return output;
}

// Per-instance hit flash — lerp toward hitFlash.rgb by intensity (w). Mirrors
// shared.hlsl ApplyHitFlash but reads the value from the vertex stream instead
// of the shared material cbuffer, so each instance flashes independently.
float3 ApplyHitFlashInst(float3 vBaseColor, float4 vHitFlash)
{
    return lerp(vBaseColor, vHitFlash.rgb, saturate(vHitFlash.a));
}

// Per-instance dissolve. paperTime drives a banded colour transition then a
// clip(), mirroring shared.hlsl GetPaperBurnColor with the enemy curve baked
// in (Attackable's setup: maxTime 4, rates 0.1/0.2/0.75, red→yellow→white).
// The engine's t4 noise texture isn't bound on the instanced path, so a cheap
// hash from uv stands in for the spatial variation.
float4 ApplyDissolveInst(float4 color, float2 uv, float paperTime)
{
    if (paperTime <= 0.f)
        return color;

    const float fMaxTime   = 4.f;
    const float fStartRate = 0.1f;
    const float fMidRate   = 0.2f;
    const float fFinalRate = 0.75f;
    const float fEndRate   = 1.0f;
    const float4 vStart = float4(1.f, 0.f, 0.f, 1.f);   // red
    const float4 vMid   = float4(1.f, 1.f, 0.f, 1.f);   // yellow
    const float4 vFinal = float4(1.f, 1.f, 1.f, 1.f);   // white

    float noise = frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
    float fRate = paperTime / fMaxTime * 3.0f - 1.f + noise;

    if (fRate < fStartRate)
        return color;
    else if (fRate < fMidRate)
    {
        float b = (fRate - fStartRate) / (fMidRate - fStartRate);
        return color * (1.0f - b) + vStart * b;
    }
    else if (fRate < fFinalRate)
    {
        float b = (fRate - fMidRate) / (fFinalRate - fMidRate);
        return vStart * (1.0f - b) + vMid * b;
    }
    else if (fRate < fEndRate)
    {
        float b = (fRate - fFinalRate) / (fEndRate - fFinalRate);
        return vMid * (1.0f - b) + vFinal * b;
    }

    clip(-1);   // fully burned → discard the pixel
    return 0.f;
}

PSOut EnemyPSInst(VSEnemyInstOut input)
{
    PSOut output;

    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    float4 albedo = ApplyDissolveInst(input.vDiffuseColor, input.uv, input.paperTime);

    output.value0.xyz = albedo.xyz;
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;

    output.value2.xyz = 0.f;
    output.value2.w = input.fMaterialFraction;

    output.value3.xyz = input.vSpecularColor.xyz;

    output.value4.xyz = 0.f;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlashInst(output.value0.xyz, input.hitFlash);
    output.value3.w = EncodeShadingId();

    return output;
}

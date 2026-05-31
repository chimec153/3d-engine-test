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

// Banded dissolve (red→yellow→white then clip). paperTime is on an enemy-tuned
// [0,4] scale; a uv hash supplies spatial noise so no t4 texture is needed.
// Shared by the solo (EnemyPS, gated by g_bMaterialUsePaperBurn) and instanced
// (EnemyPSInst, per-instance paperTime) paths.
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

// Smooth 2D value noise — used by the shard dissolve below so the burn eats the
// surface in coherent blotches instead of the per-pixel static a raw hash gives.
float FragHash21(float2 p) { return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f); }
float FragVNoise2(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.f - 2.f * f);
    float a = FragHash21(i);
    float b = FragHash21(i + float2(1.f, 0.f));
    float c = FragHash21(i + float2(0.f, 1.f));
    float d = FragHash21(i + float2(1.f, 1.f));
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

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

    // Solo dissolve, driven by the PaperBurn component's b10 cbuffer and gated
    // by the material flag (enemies don't set it, so they're untouched). This is
    // what lets fragment shards — which always render solo, never batched —
    // dissolve consistently instead of only when 2+ happen to instance together.
    // The burn edge is written to the EMISSIVE GBuffer (value4) so it glows /
    // blooms in HDR rather than being a flatly-lit colour.
    if (g_bMaterialUsePaperBurn)
    {
        float fRatio = g_fPaperTime / max(g_fPaperMaxTime, 0.0001f);
        // uv * N controls grain — bigger N = finer/tighter dissolve specks.
        float fNoise = FragVNoise2(input.uv * 4.0f);
        float fRate  = fRatio * 3.0f - 1.0f + fNoise;   // <0 intact … ≥1 burned

        if (fRate >= 1.0f)
            clip(-1);                                   // fully burned → gone

        float  fEdge   = saturate(fRate);               // 0 intact → 1 at the cut
        float3 vBurn   = lerp(float3(1.0f, 0.40f, 0.06f),   // orange
                              float3(1.0f, 1.0f, 0.70f),    // white-hot
                              fEdge);
        output.value0.xyz  = lerp(output.value0.xyz, vBurn, fEdge);   // char the surface
        output.value4.xyz += vBurn * (fEdge * fEdge) * 1.5f;          // emissive rim (HDR → bloom)
    }

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
    float  burnRim     : BurnRim;      // byte 256  (game-only)
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
    float3 viewPos      : ViewPos;     // view-space position (for Fresnel rim)
    float  burnRim      : BurnRim;
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
    output.viewPos = mul(float4(input.pos, 1.f), input.WV).xyz;
    output.burnRim = input.burnRim;
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

// Burn-rim emissive — a Fresnel (silhouette) glow that replaces the flat orange
// hit-flash tint for the Burn status. Brightest where the view-space normal is
// perpendicular to the view direction (the mesh's edges), so the body looks
// wreathed in fire. burnRim (0..1) gates + scales it; 0 = off. Written straight
// to the emissive GBuffer so it glows regardless of scene lighting.
float3 BurnRimEmissive(float3 vViewNormal, float3 vViewPos, float burnRim)
{
    if (burnRim <= 0.f)
        return 0.f;

    const float3 kRimColor    = float3(1.0f, 0.35f, 0.06f);  // warm orange
    const float  kRimPower    = 2.5f;   // higher = tighter rim
    const float  kRimStrength = 3.5f;   // HDR emissive gain

    float3 N = normalize(vViewNormal);
    float3 V = normalize(-vViewPos);                 // view space: camera at origin
    float  f = pow(1.0f - saturate(dot(N, V)), kRimPower);
    return kRimColor * f * burnRim * kRimStrength;
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

    output.value4.xyz = BurnRimEmissive(input.normal, input.viewPos, input.burnRim);
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlashInst(output.value0.xyz, input.hitFlash);
    output.value3.w = EncodeShadingId();

    return output;
}

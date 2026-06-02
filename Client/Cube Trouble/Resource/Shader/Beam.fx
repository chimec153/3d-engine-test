#include "shared.hlsl"

// ------------------------------------------------------------------------
// Laser beam — camera-facing billboard.
//
// Geometry is built CPU-side in BeamRenderManager: each beam segment becomes
// a quad whose width axis is cross(beamDir, eyeToBeam) (a cylindrical
// billboard that rotates around the beam axis to face the camera). Corners
// arrive here already in world space with a per-vertex HDR colour (the layer's
// colour * intensity, which may exceed 1.0 so the bloom post-process haloes
// it). The VS only projects; the PS shapes the cross-section, scrolls energy
// along the length, and fades against scene depth (soft particles) so the beam
// doesn't hard-edge where it meets walls / the floor.
//
// Drawn in the ALPHA pass with additive (ONE/ONE) blend + no depth write.
// Runtime-compiled (.fx), so this can be iterated without a C++ rebuild.
// ------------------------------------------------------------------------

struct BeamVSIn
{
    float3 pos   : Position;   // world space
    float2 uv    : Texcoord;   // u = along length, v = across width (0..1)
    float4 color : Color;      // rgb = colour * intensity (HDR); a unused
};

struct BeamVSOut
{
    float4 pos   : SV_POSITION;
    float2 uv    : Texcoord;
    float4 color : Color;
};

BeamVSOut VS_Beam(BeamVSIn input)
{
    BeamVSOut o = (BeamVSOut)0;
    // g_matTransform = VP — BeamRenderManager binds an identity NORMAL-camera
    // transform, so a world-space corner projects straight to clip space.
    o.pos   = mul(float4(input.pos, 1.f), g_matTransform);
    o.uv    = input.uv;
    o.color = input.color;
    return o;
}

// Cross-section sharpness — higher pulls the bright line tighter to the
// centre. One value serves every layer: the wide-soft-glow vs thin-hot-core
// look comes from per-layer width + colour set on the CPU, so the same shader
// is reused for all layers (only beamWidth / colour change).
static const float kCoreSharpness = 1.8f;

// Soft-particle fade band in view-space (world) units. The beam fades out over
// this distance as it nears scene geometry, killing the hard intersection
// edge. It doubles as occlusion: a pixel fully behind geometry fades to 0.
static const float kSoftFade = 1.5f;

// Soft particle + occlusion. Linearise the sampled scene depth and this pixel's
// own depth, then fade as they converge. g_vProjectValues (b3) is valid here
// because the light pass, which fills it, runs before the alpha pass. Empty sky
// (raw == 1) or an unbound depth SRV (raw == 0) both fall back to fully visible
// so the effect is never silently blanked. Shared by beams and bullet trails.
float BeamSoftFade(float2 svxy, float svz)
{
    float fRaw = g_DepthTexture0.Load(int3(int2(svxy), 0)).r;
    if (fRaw <= 1e-5f || fRaw >= 1.f) return 1.f;
    float fSceneZ = ConvertZToLinearDepth(fRaw);
    float fThisZ  = ConvertZToLinearDepth(svz);
    return saturate((fSceneZ - fThisZ) / kSoftFade);
}

float4 PS_Beam(BeamVSOut input) : SV_TARGET
{
    // Cross-section falloff: v = 0.5 is the centre line, v = 0/1 the edges.
    float fEdge  = saturate(1.f - abs(2.f * input.uv.y - 1.f));
    float fCross = pow(fEdge, kCoreSharpness);

    // Scrolling energy along the length, from the global noise texture (t17).
    // A subtle multiplier so the beam reads as live plasma rather than a
    // flat gradient. If the noise SRV isn't bound, .r reads 0 -> 0.75 (dim
    // but still visible), so this never blanks the beam. For bullet trails the
    // u coordinate is accumulated path distance, so the texture flows evenly.
    float fNoise  = g_NoiseTexture.Sample(g_sLinear,
                        float2(input.uv.x * 2.f - g_fGlobalAccTime * 1.5f,
                               input.uv.y * 0.5f)).r;
    float fEnergy = lerp(0.75f, 1.25f, fNoise);

    float fSoft = BeamSoftFade(input.pos.xy, input.pos.z);

    float3 rgb = input.color.rgb * fCross * fEnergy * fSoft;
    // Additive (ONE/ONE): RGB accumulates, alpha is ignored by the blend.
    return float4(rgb, 1.f);
}

// Round additive glow sprite — the bright "tracer tip" at the head of a bullet
// trail. A camera-facing quad whose uv 0..1 the VS passes through; here it
// becomes a radial falloff. Reuses BeamVS (pos+uv+colour) and the same soft
// depth fade so the dot doesn't punch through walls.
float4 PS_BeamGlow(BeamVSOut input) : SV_TARGET
{
    float2 d    = input.uv * 2.f - 1.f;            // -1..1 across the quad
    float  fR   = length(d);

    // Two layers, additively composited, give the classic tracer look:
    //  - glow: a wide, soft, fully-coloured halo (the bullet's hue).
    //  - core: a tight, near-white hot centre. A pure-colour bright dot still
    //    reads as "just a brighter blue/red"; whitening the core is what makes
    //    it read as a high-energy source. We whiten toward the layer's own HDR
    //    magnitude (max channel) so every bullet colour gets an equally hot
    //    centre that the bloom post-process haloes.
    float  fGlow = pow(saturate(1.f - fR), 2.2f);  // wide coloured halo
    float  fCore = pow(saturate(1.f - fR), 7.0f);  // tight hot core
    float  fMag  = max(input.color.r, max(input.color.g, input.color.b));
    float3 vCore = lerp(input.color.rgb, fMag.xxx, 0.85f);

    float fSoft = BeamSoftFade(input.pos.xy, input.pos.z);

    float3 rgb = (input.color.rgb * fGlow + vCore * fCore * 1.25f) * fSoft;
    return float4(rgb, 1.f);
}

// ---- Stylized enemy-death billboards -----------------------------------
// Mask (shape) and colour are separated: the shape PS computes a coverage
// mask from uv, then pulls colour + alpha from a 1xN gradient ramp LUT
// (bound at t0) sampled by the particle's normalised lifetime, which the
// vertex carries in color.a (0 = birth, 1 = death). Point sampling keeps the
// ramp's hard steps -> flat toon colour banding over life. color.rgb is an
// optional tint (white = ramp as-authored). Swapping the bound ramp per enemy
// type recolours the whole effect with the same masks. All fade against scene
// depth via BeamSoftFade so they don't punch through walls / floor.

// Lifetime -> ramp colour+alpha. g_sPoint = crisp stepped bands (toon look).
float4 SampleDeathRamp(float t)
{
    return g_Texture.Sample(g_sPoint, float2(saturate(t), 0.5f));
}

// Flat-colour puff — a solid disc with a thin anti-aliased rim (no gradient
// fill, so it reads as a flat cartoon puff). ALPHA blend.
float4 PS_DeathPuff(BeamVSOut input) : SV_TARGET
{
    float2 d = input.uv * 2.f - 1.f;
    float  r = length(d);
    float  cover = saturate((1.f - r) * 6.f);      // flat fill, crisp-ish edge
    float4 ramp = SampleDeathRamp(input.color.a);
    float  a = cover * ramp.a * BeamSoftFade(input.pos.xy, input.pos.z);
    return float4(ramp.rgb * input.color.rgb, a);
}

// 4-point sparkle star — two tapering arms (H + V) plus a bright core. ADDITIVE.
float4 PS_DeathStar(BeamVSOut input) : SV_TARGET
{
    float2 p  = input.uv * 2.f - 1.f;
    float  ax = abs(p.x), ay = abs(p.y);
    float armH = saturate(1.f - ay * 7.f) * saturate(1.f - ax);   // horizontal spike
    float armV = saturate(1.f - ax * 7.f) * saturate(1.f - ay);   // vertical spike
    float core = saturate(1.f - length(p) * 3.f);
    float cover = saturate(max(max(armH, armV), core));
    float4 ramp = SampleDeathRamp(input.color.a);
    float  k = cover * ramp.a * BeamSoftFade(input.pos.xy, input.pos.z);
    return float4(ramp.rgb * input.color.rgb * k, 1.f);          // additive
}

// Diamond sparkle — a filled rotated square (|x|+|y| <= 1). ADDITIVE.
float4 PS_DeathDiamond(BeamVSOut input) : SV_TARGET
{
    float2 p = abs(input.uv * 2.f - 1.f);
    float  cover = saturate((1.f - (p.x + p.y)) * 4.f);
    float4 ramp = SampleDeathRamp(input.color.a);
    float  k = cover * ramp.a * BeamSoftFade(input.pos.xy, input.pos.z);
    return float4(ramp.rgb * input.color.rgb * k, 1.f);          // additive
}

// Hard-edge smoke ring — a crisp annulus (no soft falloff). ALPHA blend; the
// quad scales up over the effect's life so the ring expands.
float4 PS_DeathRing(BeamVSOut input) : SV_TARGET
{
    float2 d = input.uv * 2.f - 1.f;
    float  r = length(d);
    const float inner = 0.70f, outer = 0.97f;
    float cover = saturate((r - inner) * 36.f) * saturate((outer - r) * 36.f);  // hard ring
    float4 ramp = SampleDeathRamp(input.color.a);
    float  a = cover * ramp.a * BeamSoftFade(input.pos.xy, input.pos.z);
    return float4(ramp.rgb * input.color.rgb, a);
}

// Procedural muzzle flash — an 8-spoke polar starburst drawn from uv with no
// texture. The CPU bakes a fast-decay life envelope into color.rgb (HDR, so the
// core feeds bloom) and a per-flash random seed into color.a (0..1) that rotates
// the spokes so repeated shots differ. Camera-facing quad; reuses VS_Beam /
// BeamVtx. ADDITIVE (ONE/ONE) — alpha is ignored by the blend.
float4 PS_Muzzle(BeamVSOut input) : SV_TARGET
{
    // uv.x runs along the fire direction (0 = barrel side, 1 = forward), uv.y
    // across it. The CPU orients + elongates the quad so +X here is the aim, so
    // the flash blasts one way instead of being a symmetric star.
    float2 p = input.uv * 2.f - 1.f;
    float  r = length(p);
    float  a = atan2(p.y, p.x);                 // 0 = forward, +-PI = behind
    float  fwd = saturate(p.x / (r + 1e-3f));   // forward hemisphere mask (cos a)

    // Forward cone -- a teardrop of flame blasting out of the barrel; nothing
    // behind (fwd = 0 there).
    float cone = saturate(1.f - r) * pow(fwd, 1.5f);

    // A few sharp forward spikes, jittered per-flash by the seed (color.a).
    float spikes = pow(abs(cos(a * 3.f + input.color.a * 6.2831853f)), 10.f)
                   * pow(fwd, 2.f) * saturate(1.f - r);

    // Hot core at the muzzle (quad centre) -- a small bright pop at the barrel.
    float core = pow(saturate(1.f - r * 1.8f), 4.f);

    float intensity = saturate(core + cone + spikes);
    float fSoft = BeamSoftFade(input.pos.xy, input.pos.z);
    return float4(input.color.rgb * intensity * fSoft, 1.f);                   // additive
}

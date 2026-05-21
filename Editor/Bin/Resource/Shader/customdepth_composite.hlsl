// CustomDepth composite — Unreal-style "see character through walls".
//
// Runs after the lit scene is in m_pHDRTexture, before RenderAlpha.
// Samples scene depth (t10) and the custom depth target (t19) that
// flagged MeshRendererComponents rendered into during RenderCustomDepth.
//
// Pixel selection:
//   - customZ >= 1.0 (or very close) → no flagged mesh covers this pixel,
//     skip via discard.
//   - customZ <= sceneZ → flagged mesh is in front of the scene at this
//     pixel; it's already visible through the normal opaque pass. Skip.
//   - customZ >  sceneZ → flagged mesh is behind the opaque scene at this
//     pixel; this is the "occluded" region we want to surface.
//
// Reuses MultiVS (VS_Multi in anisotropic_microfacet.hlsl) for the
// fullscreen quad, so this file only contains the pixel shader.

#include "shared.hlsl"

Texture2D<float> g_CustomDepthTexture : register(t19);

float4 PS_CustomDepthComposite(VSMultiOut input) : SV_TARGET
{
    float sceneZ  = g_DepthTexture0.Sample(g_sPoint, input.uv).r;
    float customZ = g_CustomDepthTexture.Sample(g_sPoint, input.uv).r;

    // No flagged mesh at this pixel.
    if (customZ >= 0.99999f) discard;

    // Visible portion — already rendered in the normal opaque pass.
    if (customZ <= sceneZ) discard;

    // Occluded portion — overlay a translucent silhouette so the player
    // stays readable behind voxel walls. Tweak color/alpha to taste.
    return float4(0.2f, 0.6f, 1.0f, 0.55f);
}

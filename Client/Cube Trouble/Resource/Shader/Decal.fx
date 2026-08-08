#include "shared.hlsl"

VS_DECAL_OUT VS_DECAL(VSStandardIn input)
{
    VS_DECAL_OUT output;

    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    output.screenpos = output.pos;
    output.uv = input.uv;
    output.localpos = input.pos;

    return output;
}

VS_DECAL_INST_OUT VS_DECAL_INST(VSDecalInstIn input)
{
    VS_DECAL_INST_OUT output;

    output.pos = mul(float4(input.pos, 1.f), input.WVP);
    output.screenpos = output.pos;
    output.uv = input.uv;
    output.localpos = input.pos;
    output.invWV = input.matInvWorldView;
    output.diffuse = input.vDiffuseColor;
    output.specular = input.vSpecularColor;
    output.emissive = input.vEmissiveColor;
    output.fadestart = input.fDecalFadeStart;
    output.fademax = input.fDecalFadeMax;
    output.fadetime = input.fDecalFadeTime;
    output.roughness = input.vMaterialRoughness;
    output.fraction = input.fMaterialFraction;

    return output;
}

float2 GetDecalUV(float2 uv, matrix matInvWorldView)
{
    float2 depth_uv = uv;

    depth_uv.y *= -1;

    depth_uv = depth_uv * 0.5f + 0.5f;

    float depth = g_DepthTexture0.Sample(g_sPoint, depth_uv).x;

    float3 viewpos = float3(uv, g_matProj[3][2] / (depth - g_matProj[2][2]));

    viewpos.x /= g_matProj[0][0];
    viewpos.y /= g_matProj[1][1];

    viewpos.xy *= viewpos.z;

    float3 localpos = mul(float4(viewpos, 1.f), matInvWorldView);

    float2 decal_uv = localpos.xz + 0.5f;
    clip(decal_uv);
    clip(1.0 - decal_uv);

    return decal_uv;
}

PSOut PS_DECAL(VS_DECAL_OUT input)
{
    PSOut output;

    float2 decal_uv = GetDecalUV(input.screenpos.xy / input.screenpos.w, g_matInvWorldView);

    float4 decal_diffuse = g_Texture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_normal = g_NormalTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_specular = g_SpecularTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_emissive = g_EmissiveTexture.Sample(g_sAnisotropic, decal_uv);

    float fFadeRate = clamp((g_fDecalMaxFade - g_fDecalFadeTime) / (g_fDecalMaxFade - g_fDecalFadeStart), 0.0, 1.0);

    output.value0 = (g_vDiffuseColor * decal_diffuse + g_vEmissiveColor * decal_emissive);
    output.value1.xyz = BumpMapping(float3(0.f, 1.f, 0.f), float4(1.f, 0.f, 0.f, 1.f), decal_normal.xyz) * 0.5f + 0.5f;
    output.value1.w = decal_normal.w * fFadeRate;
    output.value2.xy = g_vMaterialRoughness.xy;
    output.value2.z = g_fMaterialFraction;
    output.value2.w = fFadeRate;
    output.value3 = decal_specular * g_vSpecularColor;

    output.value0.w *= fFadeRate;
    output.value3.w *= fFadeRate;

    output.value4 = 0;
    return output;
}

// ---------------------------------------------------------------------------
// Flat PROCEDURAL floor decals (heal-tower telegraph). No texture: the ring /
// disc shape is computed from the in-circle UV, so the decals carry no asset
// and stay crisp at any scale.
//
// FloorDecalUV does the shared work: reconstruct the depth-buffer hit point,
// clip it to the box footprint + a thin floor BAND on localpos.y, and reject
// non-up-facing surfaces. A Decal component leaves the normal/spec/emissive
// slots unbound (PS_DECAL is the full-material blood decal), so these writers
// emit a clean up-normal + neutral material instead of reading those slots.
//
// Why the masks: the shared GetDecalUV clips XZ only, so a tall box would paint
// the player/monsters standing in the circle. The Y band rejects wrong-height
// surfaces; the world-normal check then rejects the up-band's remaining
// vertical bodies (a body's feet sit in the floor band but face sideways). The
// normal is taken in WORLD space (localpos is warped by the box's non-uniform
// scale) from screen derivatives, oriented toward the camera, kept only if
// up-facing. Depth-based, so instanced enemies are covered (no stencil needed).
float2 FloorDecalUV(float2 screen_uv)
{
    float2 depth_uv = screen_uv;
    depth_uv.y *= -1;
    depth_uv = depth_uv * 0.5f + 0.5f;

    float depth = g_DepthTexture0.Sample(g_sPoint, depth_uv).x;

    float3 viewpos = float3(screen_uv, g_matProj[3][2] / (depth - g_matProj[2][2]));
    viewpos.x /= g_matProj[0][0];
    viewpos.y /= g_matProj[1][1];
    viewpos.xy *= viewpos.z;

    float3 localpos = mul(float4(viewpos, 1.f), g_matInvWorldView);

    float2 decal_uv = localpos.xz + 0.5f;
    clip(decal_uv);
    clip(1.0 - decal_uv);
    clip(0.15 - abs(localpos.y));   // floor band: reject wrong-height surfaces

    float3 worldpos = mul(float4(viewpos, 1.f), g_matInvView).xyz;
    float3 wn = normalize(cross(ddy(worldpos), ddx(worldpos)));
    float3 camPos = float3(g_matInvView[3][0], g_matInvView[3][1], g_matInvView[3][2]);
    if (dot(wn, normalize(camPos - worldpos)) < 0.f) wn = -wn;   // face the camera
    clip(wn.y - 0.6f);                                           // up-facing floor only

    return decal_uv;
}

PSOut WriteFloorDecal(float alpha)
{
    PSOut output = (PSOut) 0;
    float fFadeRate = clamp((g_fDecalMaxFade - g_fDecalFadeTime) / (g_fDecalMaxFade - g_fDecalFadeStart), 0.0, 1.0);
    float a = alpha * fFadeRate;

    output.value0 = float4(g_vDiffuseColor.rgb, a);
    output.value1 = float4(0.5, 1.0, 0.5, a);   // encoded up normal (0,1,0)
    output.value2 = float4(g_vMaterialRoughness.x, g_vMaterialRoughness.y, g_fMaterialFraction, a);
    output.value3 = float4(0, 0, 0, 0);
    return output;
}

// Bright annulus at the rim (the static outer ring).
PSOut PS_DECAL_RING(VS_DECAL_OUT input)
{
    float2 decal_uv = FloorDecalUV(input.screenpos.xy / input.screenpos.w);
    float r = length(decal_uv - 0.5f); // 0 centre .. 1 edge
    // static outer ring at the boundary (r ~ 0.5)
    float ring = smoothstep(0.42, 0.44, r) * (1.0 - smoothstep(0.49, 0.5, r));
    float CastingProgress = 1.0f - clamp((g_fDecalMaxFade - g_fDecalFadeTime) / (g_fDecalMaxFade - g_fDecalFadeStart), 0.0, 1.0);
    float currentRadius = CastingProgress * 0.46f; // fill grows up to inner edge of ring
    
    float disk = 1.0 - smoothstep(currentRadius - 0.02, currentRadius, r);
    float progressMask = max(ring, disk); // ring + disk combined, same color
    
    if (progressMask < 0.9f)
    {
        clip(-1);
    }
    
    float3 diskColor = float3(0.5, 1.0, 0.5); // orange fill

    PSOut output = (PSOut) 0;
    output.value0 = float4(diskColor, progressMask); // albedo across whole decal (no dark rim)
    //output.value1 = float4(0.5, 1.0, 0.5, 0.f); // encoded up normal (0,1,0)
    //output.value4 = float4(diskColor, progressMask); // emissive: yellow glowing ring (MRT4)
    return output;
}

PSOut PS_DECAL_INST(VS_DECAL_INST_OUT input)
{
    PSOut output;

    float2 decal_uv = GetDecalUV(input.screenpos.xy / input.screenpos.w, input.invWV);

    float4 decal_diffuse = g_Texture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_normal = g_NormalTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_specular = g_SpecularTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_emissive = g_EmissiveTexture.Sample(g_sAnisotropic, decal_uv);

    float fFadeRate = clamp((input.fademax - input.fadetime) / (input.fademax - input.fadestart), 0.0, 1.0);

    output.value0 = (input.diffuse * decal_diffuse + input.emissive * decal_emissive);
    output.value1.xyz = BumpMapping(float3(0.f, 1.f, 0.f), float4(1.f, 0.f, 0.f, 1.f), decal_normal.xyz) * 0.5f + 0.5f;
    output.value1.w = decal_normal.w * fFadeRate;
    output.value2.xy = input.roughness;
    output.value2.z = input.fraction;
    output.value2.w = fFadeRate;
    output.value3 = decal_specular * input.specular;

    output.value0.w *= fFadeRate;
    output.value3.w *= fFadeRate;

    output.value4 = 0;
    return output;
}

PSOut PS_DECAL_PBR(VS_DECAL_OUT input)
{
    PSOut output;

    float2 decal_uv = GetDecalUV(input.screenpos.xy / input.screenpos.w, g_matInvWorldView);

    float4 decal_diffuse = g_Texture.Sample(g_sAnisotropic, decal_uv);

    float4 decal_normal = g_NormalTexture.Sample(g_sAnisotropic, decal_uv);

    float opacity = g_SpecularTexture.Sample(g_sAnisotropic, decal_uv).r; // opacity

    if(opacity == 0.0)
    {
        clip(-1);
    }

    float decal_roughness = g_EmissiveTexture.Sample(g_sAnisotropic, decal_uv).r; // roughness

    float fFadeRate = clamp((g_fDecalMaxFade - g_fDecalFadeTime) / (g_fDecalMaxFade - g_fDecalFadeStart), 0.0, 1.0);

    output.value0 = g_vDiffuseColor * decal_diffuse;
    output.value1.xyz = BumpMapping(float3(0.f, 1.f, 0.f), float4(1.f, 0.f, 0.f, 1.f), decal_normal.xyz) * 0.5f + 0.5f;
    output.value1.w = decal_normal.w * fFadeRate * opacity;
    output.value2.xy = g_vMaterialRoughness.xy * decal_roughness;
    output.value2.z = g_fMaterialFraction;
    output.value2.w = fFadeRate;
    output.value3 = g_vSpecularColor;

    output.value0.w *= fFadeRate;
    output.value3.w *= fFadeRate;

    output.value4 = 0;
    return output;
}

PSOut PS_DECAL_PBR_INST(VS_DECAL_INST_OUT input)
{
    PSOut output;

    float2 decal_uv = GetDecalUV(input.screenpos.xy / input.screenpos.w, input.invWV);

    float4 decal_diffuse = g_Texture.Sample(g_sAnisotropic, decal_uv);

    float4 decal_normal = g_NormalTexture.Sample(g_sAnisotropic, decal_uv);

    float opacity = g_SpecularTexture.Sample(g_sAnisotropic, decal_uv).r; // opacity

    if (opacity == 0.0)
    {
        clip(-1);
    }

    float decal_roughness = g_EmissiveTexture.Sample(g_sAnisotropic, decal_uv).r; // roughness

    float fFadeRate = clamp((input.fademax - input.fadetime) / (input.fademax - input.fadestart), 0.0, 1.0);

    output.value0 = input.diffuse * decal_diffuse;
    output.value1.xyz = BumpMapping(float3(0.f, 1.f, 0.f), float4(1.f, 0.f, 0.f, 1.f), decal_normal.xyz) * 0.5f + 0.5f;
    output.value1.w = decal_normal.w * fFadeRate * opacity;
    output.value2.xy = input.roughness * decal_roughness;
    output.value2.z = input.fraction;
    output.value2.w = fFadeRate;
    output.value3 = input.specular;

    output.value0.w *= fFadeRate;
    output.value3.w *= fFadeRate;

    output.value4 = 0;
    return output;
}

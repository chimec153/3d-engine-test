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

PSOut PS_DECAL(VS_DECAL_OUT input)
{
    PSOut output;
    
    float2 depth_uv = input.screenpos.xy / input.screenpos.w;
    
    depth_uv.y *= -1;
    
    depth_uv = depth_uv * 0.5f + 0.5f;
    
    float depth = g_DepthTexture0.Sample(g_sPoint, depth_uv).x;
    
    float3 viewpos = float3(input.screenpos.xy / input.screenpos.w, g_matProj[3][2] / (depth - g_matProj[2][2]));
    
    viewpos.x /= g_matProj[0][0];
    viewpos.y /= g_matProj[1][1];
    
    viewpos.xy *= viewpos.z;
    
    float3 localpos = mul(float4(viewpos, 1.f), g_matInvWorldView);
    //clip(depth - input.screenpos.z / input.screenpos.w);
    //clip(0.5 - abs(input.localpos.xyz - localpos.xyz));
    
    float2 decal_uv = localpos.xz + 0.5f;
    clip(decal_uv);
    clip(1.0 - decal_uv);
    
    float4 decal_diffuse = g_Texture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_normal = g_NormalTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_specular = g_SpecularTexture.Sample(g_sAnisotropic, decal_uv);
    float4 decal_emissive = g_EmissiveTexture.Sample(g_sAnisotropic, decal_uv);
    
    float fFadeRate = clamp((g_fDecalMaxFade - g_fDecalFadeTime) / (g_fDecalMaxFade - g_fDecalFadeStart), 0.0, 1.0);
    
    output.value0 = (g_vDiffuseColor * decal_diffuse + g_vEmissiveColor * decal_emissive);
    output.value1.xyz = BumpMapping(float3(0.f, 1.f, 0.f), float4(1.f, 0.f, 0.f, 1.f), decal_normal.xyz) * 0.5f + 0.5f;
    output.value1.w = decal_normal.w * fFadeRate;
    output.value2 = decal_specular;
    output.value3 = g_vSpecularColor;
    
    output.value0.w *= fFadeRate;
    output.value2.w *= fFadeRate;
    output.value3.w *= fFadeRate;
    
    return output;
}
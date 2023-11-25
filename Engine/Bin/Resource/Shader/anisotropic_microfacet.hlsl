#include "shared.hlsl"

VSOut VS(VSIn input)
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

VSOut VS_NoSkin(VSStandardIn input)
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

VSInstOut VS_NoSkinInst(VSStandardInstIn input)
{
    VSInstOut output;
    
    output.pos = mul(float4(input.pos, 1.f), input.WVP);
    output.view = mul(input.pos, (float3x3) input.WV);
    output.uv = input.uv;
    
    output.normal = normalize(mul(input.normal, (float3x3) input.WV));
    
    output.tangent.xyz = normalize(mul(input.tangent.xyz, (float3x3) input.WV));
    output.tangent.w = input.tangent.w;
    
    output.vDiffuseColor = input.diffuse;
    output.vSpecularColor = input.specular;
    output.vMaterialRoughness = input.roughness;
    output.fMaterialFraction = input.fraction;
    output.clip = output.pos;
    
    return output;
}

VS_Terrain_Out VS_Terrain(VSStandardIn input)
{
    VS_Terrain_Out output;
    
    float3 pos = input.pos;
    
    output.blend_uv.x = pos.x / g_iTerrainWidth;
    output.blend_uv.y = 1.f - pos.z / g_iTerrainWidth;
    
    float3 leftpos = input.pos;
    leftpos.x -= 1.f;
    
    float2 leftuv = leftpos.xz / g_iTerrainWidth;
    leftuv.y = 1.f - leftuv.y;
    
    float3 rightpos = input.pos;
    rightpos.x += 1.f;
    
    float2 rightuv = rightpos.xz / g_iTerrainWidth;
    rightuv.y = 1.f - rightuv.y;
    
    float3 uppos = input.pos;
    uppos.z += 1.f;
    
    float2 upuv = uppos.xz / g_iTerrainWidth;
    upuv.y = 1.f - upuv.y;
    
    float3 downpos = input.pos;
    downpos.z -= 1.f;
    
    float2 downuv = downpos.xz / g_iTerrainWidth;
    downuv.y = 1.f - downuv.y;
    
    pos.y = g_HeightTexture.SampleLevel(g_sPoint, output.blend_uv, 0.f).r * 10.f;
    leftpos.y = g_HeightTexture.SampleLevel(g_sAnisotropic, leftuv, 0.f).r * 10.f;
    rightpos.y = g_HeightTexture.SampleLevel(g_sAnisotropic, rightuv, 0.f).r * 10.f;
    uppos.y = g_HeightTexture.SampleLevel(g_sAnisotropic, upuv, 0.f).r * 10.f;
    downpos.y = g_HeightTexture.SampleLevel(g_sAnisotropic, downuv, 0.f).r * 10.f;
    
    float3 v1 = float3(2.f, rightpos.y - leftpos.y, 0.f);
    float3 v2 = float3(0.f, downpos.y - uppos.y, -2.f);
    
    float3 normal = normalize(cross(v1, v2));
    
    float3 tangent = 0.f;
    
    tangent.x = sqrt(1.f - (1.f / (pow(normal.y, 2.f) / pow(normal.x, 2.f))));
    tangent.y = -tangent.x * normal.x / normal.y;
    
    output.pos = mul(float4(pos, 1.f), g_matTransform);
    output.uv = input.uv;
    
    output.normal = normalize(mul(normal, (float3x3) g_matWorldView));
    
    output.tangent.xyz = normalize(mul(tangent, (float3x3) g_matWorldView));
    output.tangent.w = input.tangent.w;
    
    return output;
}

VSOut VS_Skin(VSStandardIn input)
{
    VSOut output;
    
    float4 pos = 0.f;
    float3 normal = 0.f;
    float3 tangent = 0.f;
    
    float fWeight[4] = { input.blendWeight[0], input.blendWeight[1], input.blendWeight[2], 1.f - input.blendWeight[0] - input.blendWeight[1] - input.blendWeight[2]};
    
    [unroll]
    for (int i = 0; i < 4;++i)
    {        
        pos += mul(float4(input.pos, 1.f), g_vecBones[input.blendIndex[i]]) * fWeight[i];
        normal += mul(input.normal, (float3x3) g_vecBones[input.blendIndex[i]]) * fWeight[i];
        tangent += mul(input.tangent.xyz, (float3x3) g_vecBones[input.blendIndex[i]]) * fWeight[i];
    }
    
    if(g_iTransformJointSocket == -1)
    {
        output.pos = mul(pos, g_matTransform);
        output.view = mul(pos.xyz, (float3x3)g_matWorldView);
    }
    else
    {
        output.pos = mul(pos, mul(g_matJoint, mul(g_vecJointSockets[g_iTransformJointSocket], g_matTransform)));
        output.view = mul(pos.xyz, mul((float3x3) g_matJoint, mul((float3x3) g_vecJointSockets[g_iTransformJointSocket], (float3x3) g_matWorldView)));
    }
    
    output.uv = input.uv;
    
    output.normal = normalize(mul(normal, (float3x3) g_matWorldView));
    
    output.tangent.xyz = normalize(mul(tangent, (float3x3) g_matWorldView));
    output.tangent.w = input.tangent.w;
    output.clip = output.pos;
    
    return output;
}

VSInstOut VS_SkinInst(VSStandardInstIn input, uint iInstId : SV_InstanceID)
{
    VSInstOut output;
    
    float4 pos = 0.f;
    float3 normal = 0.f;
    float3 tangent = 0.f;
    
    float fWeight[4] = { input.blendWeight[0], input.blendWeight[1], input.blendWeight[2], 1.f - input.blendWeight[0] - input.blendWeight[1] - input.blendWeight[2] };
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        pos += mul(float4(input.pos, 1.f), g_vecBones[input.blendIndex[i] + g_iBoneMaxJoint * iInstId]) * fWeight[i];
        normal += mul(input.normal, (float3x3) g_vecBones[input.blendIndex[i] + g_iBoneMaxJoint * iInstId]) * fWeight[i];
        tangent += mul(input.tangent.xyz, (float3x3) g_vecBones[input.blendIndex[i] + g_iBoneMaxJoint * iInstId]) * fWeight[i];
    }
    
    matrix matWVP = input.WVP;
    float3x3 matWV = (float3x3)input.WV;
    
    if(input.parentJoint != -1)
    {
        matWVP = mul(input.joint, mul(g_vecJointSockets[input.parentJoint + input.parentJointCount * input.instID], matWVP));
        matWV = mul((float3x3) input.joint, mul((float3x3) g_vecJointSockets[input.parentJoint + input.parentJointCount * input.instID], (float3x3) matWV));
    }
    
    output.pos = mul(pos, matWVP);
    output.view = mul(pos.xyz, matWV);
    output.uv = input.uv;
    
    output.normal = normalize(mul(normal, (float3x3) input.WV));
    
    output.tangent.xyz = normalize(mul(tangent, (float3x3) input.WV));
    output.tangent.w = input.tangent.w;
    
    output.vDiffuseColor = g_vDiffuseColor;
    output.vSpecularColor = g_vSpecularColor;
    output.vMaterialRoughness = g_vMaterialRoughness;
    output.fMaterialFraction = g_fMaterialFraction;
    output.clip = output.pos;
    
    return output;
}

float4 GetFresnel(float LDotH, float4 vSpecColor)
{
    float4 rt = sqrt(vSpecColor);
    
    float4 etha = (1.f + rt) / (1.f - rt);
    
    float4 g = sqrt(etha * etha - 1.f + LDotH * LDotH);
    
    return (g - LDotH) * (g - LDotH) / (g + LDotH) / (g + LDotH) *
    ((LDotH * (g + LDotH) - 1.f) * (LDotH * (g + LDotH) - 1.f) / (LDotH * (g - LDotH) + 1.f) / (LDotH * (g - LDotH) + 1.f) + 1.f) / 2.f;
}

float3 GetFresnel(float LDotH, float3 vSpecColor)
{
    float3 rt = sqrt(vSpecColor);
    
    float3 etha = (1.f + rt) / (1.f - rt);
    
    float3 g = sqrt(etha * etha - 1.f + LDotH * LDotH);
    
    return (g - LDotH) * (g - LDotH) / (g + LDotH) / (g + LDotH) * 
    ((LDotH * (g + LDotH) - 1.f) * (LDotH * (g + LDotH) - 1.f) / (LDotH * (g - LDotH) + 1.f) / (LDotH * (g - LDotH) + 1.f) + 1.f) / 2.f;
}

float4 GetMicrofacetDistribution(float NDotH, float2 vMaterialRoughness)
{
    return 1.f / 4.f / vMaterialRoughness.x / vMaterialRoughness.x * exp((NDotH * NDotH - 1.f) / (vMaterialRoughness.x * vMaterialRoughness.x * NDotH * NDotH));
}

float4 GetMicrofacetDistribution(float NDotH, float TDotPPow2, float2 vMaterialRoughness)
{
    return 1.f / 4.f / vMaterialRoughness.x / vMaterialRoughness.y * exp((TDotPPow2 / vMaterialRoughness.x / vMaterialRoughness.x + (1.f - TDotPPow2) / vMaterialRoughness.y / vMaterialRoughness.y) *
    (NDotH * NDotH - 1.f) / (NDotH * NDotH));
}

float4 GetGeometricAttenuation(float NDotH, float NDotV, float NDotL, float LDotH)
{
    return min(min(1.f, 2.f * NDotH * NDotV / LDotH), 2.f * NDotH * NDotL / LDotH);
}

PSOut PS(VSOut input)
{
    PSOut output;
    
    output.value0.xyz = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz + g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    output.value2.w = g_fMaterialFraction;
    
    return output;
}

PSOut PS_NoSpecMap(VSOut input)
{
    PSOut output;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoNormalMap(VSOut input)
{
    PSOut output;
    
    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoSpecMapNoNormalMap(VSOut input)
{
    PSOut output;
    
    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoDiffuseNoSpecMapNoNormalMap(VSOut input)
{
    PSOut output;
    
    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = 0.f;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoTexture(VSOut input)
{
    PSOut output;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

PSOut PS_Terrain(VS_Terrain_Out input)
{
    PSOut output;
    
    float3 normal = normalize(input.normal);
    
    float3 tangent = normalize(input.tangent.xyz);
    
    float3 bitangent = cross(normal, tangent) * input.tangent.w;
    
    float fTotal = 0.f;
    
    float3 vNormal = 0.f;
    
    float3 vTexture = 0.f;
    
    float3 vEmissive = 0.f;
    
    float3 vSpecular = 0.f;
    
    for (int i = 0; i < g_iTerrainBlendCount;++i)
    {
        float fRate = g_BlendTerrainTexture.Sample(g_sAnisotropic, float3(input.blend_uv, i)).r;
        
        fTotal += fRate;
        
        vNormal += g_TerrainNormalTexture.Sample(g_sAnisotropic, float3(input.uv, i)).xyz * fRate;
        
        vTexture += g_TerrainTexture.Sample(g_sAnisotropic, float3(input.uv, i)).xyz * fRate;

        vEmissive += g_TerrainEmissiveTexture.Sample(g_sAnisotropic, float3(input.uv, i)).xyz * fRate;
        
        vSpecular += g_TerrainSpecularTexture.Sample(g_sAnisotropic, float3(input.uv, i)).xyz * fRate;
    }
    
    vTexture /= fTotal;
    vNormal /= fTotal;
    vEmissive /= fTotal;
    vSpecular /= fTotal;
    
    float3 N = normalize(vNormal * 2.f - 1.f);
    
    output.value1.xyz = normalize(float3(
    tangent.x * N.x + bitangent.x * N.y + normal.x * N.z,
    tangent.y * N.x + bitangent.y * N.y + normal.y * N.z,
    tangent.z * N.x + bitangent.z * N.y + normal.z * N.z)) * 0.5f + 0.5f;
    
    output.value0.xyz = g_vDiffuseColor.xyz * vTexture + g_vEmissiveColor.xyz * vEmissive;
    
    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;
    
    output.value2.xyz = vSpecular;
    
    output.value2.w = g_fMaterialFraction;
    
    output.value3.xyz = g_vSpecularColor.xyz;
    
    return output;
}

struct VSSphereIn
{
    float3 pos  :   Position;
    float3 normal : Normal;
};

struct VSSphereOut
{
    float4 pos : SV_Position;
    float3 normal : Normal;
    float3 light : Light;
    float3 view : View;
};


VSSphereOut VS_Sphere(VSSphereIn input)
{
    VSSphereOut output;
    
    output.pos = mul(float4(input.pos, 1.f), g_matTransform);
    
    float3 view = mul(float4(input.pos, 1.f), g_matWorldView).xyz;
    
    float3 normal = mul(input.normal, (float3x3) g_matWorldView);
    
    float3 toLight = g_vLightPos - view;
    
    output.normal = normal;
    output.light = toLight;
    output.view = -view;
    
    return output;
}


float4 PS_Sphere(VSSphereOut input) : SV_Target
{
    float3 N = normalize(input.normal);
    
    float3 light = 0.f;
    
    float3 view = normalize(input.view);
    
    float4 C = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(input.light);
        
        light = normalize(input.light);
    }
    else if(g_iLightType == SPOT_LIGHT)
    {
        light = normalize(input.light);
        
        C = GetLightAtt(input.light) * pow(max(dot(light, -g_vLightDir), 0.f), g_fLightIntensity);
    }
    else if(g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(input.light);
        
        C = g_vLightColor;
    }
    
    float3 hdir = normalize(light + view);
    
    float NDotH = dot(N, hdir);
    
    float NDotL = dot(N, light);
    
    float LDotH = dot(light, hdir);
    
    float NDotV = dot(N, view);
    
    float3 P = normalize(hdir - NDotH * N);
    
    float4 vFresnel = float4(GetFresnel(LDotH, g_vSpecularColor.xyz), 1.f);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / (hdir.x * hdir.x + hdir.y * hdir.y));
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    return g_fMaterialFraction * C * g_vDiffuseColor * NDotL + (1.f - g_fMaterialFraction) * C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV;
}

static const float2 g_vPosition[4] =
{
    float2(-1.f, 1.f),
    float2(1.f, 1.f),
    float2(-1.f, -1.f),
    float2(1.f, -1.f),
};

static const float2 g_vUV[4] =
{
    float2(0.f, 0.f),
    float2(1.f, 0.f),
    float2(0.f, 1.f),
    float2(1.f, 1.f),
};

VSMultiOut VS_Multi(uint index :    SV_VertexID)
{
    VSMultiOut output;
    
    output.pos = float4(g_vPosition[index], 0.f, 1.f);
    output.uv = g_vPosition[index] * 0.5f + 0.5f;
    
    output.uv.y = 1.f-output.uv.y;

    return output;
}

float4 PS_Multi(VSMultiOut input)   :   SV_TARGET
{
    float3 viewPos = 0.f;
    
    float depth = g_DepthTexture0.Sample(g_sPoint, input.uv).r;
    
    float2 pos = input.uv * 2.f - 1.f;
    
    pos.y = -pos.y;
    
    viewPos.z = g_vProjectValues.w / (g_vProjectValues.z - depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
    
    float4 shadowpos = mul(float4(viewPos, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float4 decal0 = g_DecalTexture0.Sample(g_sPoint, input.uv);
    float4 decal1 = g_DecalTexture1.Sample(g_sPoint, input.uv);
    float4 decal2 = g_DecalTexture2.Sample(g_sPoint, input.uv);
    float4 decal3 = g_DecalTexture3.Sample(g_sPoint, input.uv);
    
    float4 fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z);
    
    float4 value0 = g_GBufferTexture0.Sample(g_sPoint, input.uv);
    
    float4 value1 = g_GBufferTexture1.Sample(g_sPoint, input.uv);
    
    float2 vMaterialRoughness = float2(value0.w, value1.w);
    
    float3 albedo = value0.xyz * (1.f - decal0.w) + decal0.xyz * decal0.w;
    
    float3 normal = ((value1.xyz * (1.f - decal1.w) + decal1.xyz * decal1.w) - 0.5f) * 2.f;
    
    float4 value2 = g_GBufferTexture2.Sample(g_sPoint, input.uv);
    
    float4 value3 = g_GBufferTexture3.Sample(g_sPoint, input.uv);
    
    float3 vSpecColor = value3.xyz * (1.f - decal3.w) + decal3.xyz * decal3.w;
    
    float3 G = value2.xyz;
    
    float materialFraction = value2.w;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(g_vLightPos - viewPos) * g_fLightIntensity;
        
        light = normalize(g_vLightPos - viewPos);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(g_vLightPos - viewPos);
        
        C = GetLightAtt(g_vLightPos - viewPos) * pow(max(dot(g_vLightDir, light), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(-g_vLightDir);
        
        C = g_vLightColor * g_fLightIntensity;
    }
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = dot(normal, hdir);
    
    float NDotL = dot(normal, light);
    
    float LDotH = dot(light, hdir);
    
    float NDotV = dot(normal, view);
    
    float3 P = normalize(hdir - NDotH * normal);
    
    float4 vFresnel = float4(GetFresnel(LDotH, vSpecColor), 1.f);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / (hdir.x * hdir.x + hdir.y * hdir.y), vMaterialRoughness);
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    return (materialFraction * C * float4(albedo, 1.f) * max(NDotL, 0.f)
    + (1.f - materialFraction) * saturate(C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV)) * fShadowAttr;
}

float4 VS_PointLight()  :   SV_Position
{
    return float4(0.f, 0.f, 0.f ,1.f);
}

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

HS_CONSTANT_DATA_OUTPUT HS_PointLightConstant()
{
    HS_CONSTANT_DATA_OUTPUT output;
    
    output.Edges[0] = output.Edges[1] = output.Edges[2] = output.Edges[3] = 18.f;
    output.Inside[0] = output.Inside[1] = 18.f;
    
    return output;
}

static const float3 g_vHemilDir[2] =
{
    float3(1.f, 1.f, 1.f),
    float3(-1.f, 1.f, -1.f),
};

struct HS_Out
{
    float3 Hemildir :   DIR;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HS_PointLightConstant")]
HS_Out HS_PointLight(uint iPrimitiveID  :   SV_PrimitiveID)
{
    HS_Out output;
    
    output.Hemildir = g_vHemilDir[iPrimitiveID];
    
    return output;
}

[domain("quad")]
VSMultiOut DS_PointLight(HS_CONSTANT_DATA_OUTPUT input, float2 uv : SV_DomainLocation, const OutputPatch<HS_Out, 4> quad)
{
    VSMultiOut output;
    
    float2 pos = uv * 2.f - 1.f;
    
    float2 posAbs = abs(pos);
    
    float maxvalue = max(posAbs.x, posAbs.y);
    
    float3 normal = normalize(float3(pos, maxvalue - 1.f) * quad[0].Hemildir);
    
    output.pos = mul(float4(normal, 1.f), g_matTransform);

    output.uv = output.pos.xy / output.pos.w; 
    
    output.uv = output.uv * 0.5f + 0.5f;
    
    output.uv.y = 1.f - output.uv.y;
    
    return output;
}

VSInstOut VSInst(VSInstIn input)
{
    VSInstOut output;
    
    output.pos = mul(float4(input.pos, 1.f), input.matWVP);
    output.uv = input.uv;
    
    output.normal = normalize(mul(input.normal, (float3x3) input.matWV));
    
    output.tangent.xyz = normalize(mul(input.tangent.xyz, (float3x3) input.matWV));
    output.tangent.w = input.tangent.w;
    
    output.vDiffuseColor = input.vDiffuseColor;
    output.vSpecularColor = input.vSpecularColor;
    output.vMaterialRoughness = input.vMaterialRoughness;
    output.fMaterialFraction = input.fMaterialFraction;
    
    return output;
}

PSOut PSInst(VSInstOut input)
{
    PSOut output;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoSpecInst(VSInstOut input)
{
    PSOut output;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = 0.f;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoNormalInst(VSInstOut input)
{
    PSOut output;
    
    float3 normal = normalize(input.normal);
    
    output.value1.xyz = normal * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoSpecNoNormalInst(VSInstOut input)
{
    PSOut output;
    
    float3 normal = normalize(input.normal);
    
    output.value1.xyz = normal * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = 0.f;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoDiffuseNoSpecNoNormalInst(VSInstOut input)
{
    PSOut output;
    
    float3 normal = normalize(input.normal);
    
    output.value1.xyz = normal * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = 0.f;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

PSOut PS_NoTextureInst(VSInstOut input)
{
    PSOut output;
    
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;
    
    output.value0.xyz = input.vDiffuseColor.xyz;
    
    output.value0.w = input.vMaterialRoughness.x;
    output.value1.w = input.vMaterialRoughness.y;
    
    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;
    
    output.value2.w = input.fMaterialFraction;
    
    output.value3.xyz = input.vSpecularColor.xyz;
    
    return output;
}

float4 PS_Alpha(VSOut input) : SV_Target
{
    float3 viewPos = 0.f;
    
    float depth = input.clip.z / input.clip.w;
    
    float2 pos = input.clip.xy * 0.5f + 0.5f;
    
    pos.y = -pos.y;
    
    viewPos.z = g_vProjectValues.w / (g_vProjectValues.z - depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
    
    float4 shadowpos = mul(float4(viewPos, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float4 fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z);
    
    float2 vMaterialRoughness = g_vMaterialRoughness;
    
    float4 albedo = g_Texture.Sample(g_sAnisotropic, input.uv) * g_vDiffuseColor + g_vEmissiveColor * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv);
    
    float3 normal = BumpMapping(input.normal, input.tangent, input.uv);
    
    float4 vSpecColor = g_SpecularTexture.Sample(g_sAnisotropic, input.uv) * g_vSpecularColor;
    
    float materialFraction = g_fMaterialFraction;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(g_vLightPos - input.view) * g_fLightIntensity;
        
        light = normalize(g_vLightPos - input.view);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(g_vLightPos - input.view);
        
        C = GetLightAtt(g_vLightPos - input.view) * pow(max(dot(g_vLightDir, light), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(-g_vLightDir);
        
        C = g_vLightColor * g_fLightIntensity;
    }
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = dot(normal, hdir);
    
    float NDotL = dot(normal, light);
    
    float LDotH = dot(light, hdir);
    
    float NDotV = dot(normal, view);
    
    float3 P = normalize(hdir - NDotH * normal);
    
    float4 vFresnel = float4(GetFresnel(LDotH, vSpecColor.xyz), 1.f);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / (hdir.x * hdir.x + hdir.y * hdir.y), vMaterialRoughness);
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    return (materialFraction * C * albedo * max(NDotL, 0.f)
    + (1.f - materialFraction) * saturate(C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV)) * fShadowAttr;
}


float4 PS_AlphaNoUV(VSOut input) : SV_Target
{
    float3 viewPos = 0.f;
    
    float depth = input.clip.z / input.clip.w;
    
    float2 pos = input.clip.xy * 0.5f + 0.5f;
    
    pos.y = -pos.y;
    
    viewPos.z = g_vProjectValues.w / (g_vProjectValues.z - depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
    
    float4 shadowpos = mul(float4(viewPos, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float4 fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z);
    
    float2 vMaterialRoughness = g_vMaterialRoughness;
    
    float4 albedo = g_vDiffuseColor + g_vEmissiveColor;
    
    float3 normal = input.normal;
    
    float4 vSpecColor = g_vSpecularColor;
    
    float materialFraction = g_fMaterialFraction;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(g_vLightPos - input.view) * g_fLightIntensity;
        
        light = normalize(g_vLightPos - input.view);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(g_vLightPos - input.view);
        
        C = GetLightAtt(g_vLightPos - input.view) * pow(max(dot(g_vLightDir, light), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(-g_vLightDir);
        
        C = g_vLightColor * g_fLightIntensity;
    }
    
    float3 view = normalize(-input.view);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = dot(normal, hdir);
    
    float NDotL = dot(normal, light);
    
    float LDotH = dot(light, hdir);
    
    float NDotV = dot(normal, view);
    
    float3 P = normalize(hdir - NDotH * normal);
    
    float4 vFresnel = float4(GetFresnel(LDotH, vSpecColor.xyz), 1.f);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / (hdir.x * hdir.x + hdir.y * hdir.y), vMaterialRoughness);
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    return (materialFraction * C * albedo * max(NDotL, 0.f)
    + (1.f - materialFraction) * saturate(C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV)) * fShadowAttr;
}


float4 PS_AlphaInst(VSInstOut input) : SV_Target
{
    float3 viewPos = 0.f;
    
    float depth = input.clip.z / input.clip.w;
    
    float2 pos = input.clip.xy * 0.5f + 0.5f;
    
    pos.y = -pos.y;
    
    viewPos.z = g_vProjectValues.w / (g_vProjectValues.z - depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
    
    float4 shadowpos = mul(float4(viewPos, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float4 fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z);
    
    float2 vMaterialRoughness = input.vMaterialRoughness;
    
    float4 albedo = g_Texture.Sample(g_sAnisotropic, input.uv) * input.vDiffuseColor;
    
    float3 normal = BumpMapping(input.normal, input.tangent, input.uv);
    
    float4 vSpecColor = g_SpecularTexture.Sample(g_sAnisotropic, input.uv) * input.vSpecularColor;
    
    float materialFraction = input.fMaterialFraction;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(g_vLightPos - input.view) * g_fLightIntensity;
        
        light = normalize(g_vLightPos - input.view);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(g_vLightPos - input.view);
        
        C = GetLightAtt(g_vLightPos - input.view) * pow(max(dot(g_vLightDir, light), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(-g_vLightDir);
        
        C = g_vLightColor * g_fLightIntensity;
    }
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = dot(normal, hdir);
    
    float NDotL = dot(normal, light);
    
    float LDotH = dot(light, hdir);
    
    float NDotV = dot(normal, view);
    
    float3 P = normalize(hdir - NDotH * normal);
    
    float4 vFresnel = float4(GetFresnel(LDotH, vSpecColor.xyz), 1.f);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / (hdir.x * hdir.x + hdir.y * hdir.y), vMaterialRoughness);
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    return float4(((materialFraction * C * albedo * max(NDotL, 0.f)
    + (1.f - materialFraction) * saturate(C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV)) * fShadowAttr).xyz, albedo.w);
}
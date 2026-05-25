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
    
    // X uses Width, Z uses Height. Old code divided Z by Width too — works
    // only for square terrains and creates skewed UVs otherwise.
    output.blend_uv.x = pos.x / g_iTerrainWidth;
    output.blend_uv.y = 1.f - pos.z / g_iTerrainHeight;

    float3 leftpos = input.pos;
    leftpos.x -= 1.f;

    float2 leftuv = float2(leftpos.x / g_iTerrainWidth, 1.f - leftpos.z / g_iTerrainHeight);

    float3 rightpos = input.pos;
    rightpos.x += 1.f;

    float2 rightuv = float2(rightpos.x / g_iTerrainWidth, 1.f - rightpos.z / g_iTerrainHeight);

    float3 uppos = input.pos;
    uppos.z += 1.f;

    float2 upuv = float2(uppos.x / g_iTerrainWidth, 1.f - uppos.z / g_iTerrainHeight);

    float3 downpos = input.pos;
    downpos.z -= 1.f;

    float2 downuv = float2(downpos.x / g_iTerrainWidth, 1.f - downpos.z / g_iTerrainHeight);
    
    pos.y = g_HeightTexture.SampleLevel(g_sPoint, output.blend_uv, 0.f).r * 255.f;
    leftpos.y = g_HeightTexture.SampleLevel(g_sPoint, leftuv, 0.f).r * 255.f;
    rightpos.y = g_HeightTexture.SampleLevel(g_sPoint, rightuv, 0.f).r * 255.f;
    uppos.y = g_HeightTexture.SampleLevel(g_sPoint, upuv, 0.f).r * 255.f;
    downpos.y = g_HeightTexture.SampleLevel(g_sPoint, downuv, 0.f).r * 255.f;
    
    float3 v1 = normalize(float3(2.f, rightpos.y - leftpos.y, 0.f));
    float3 v2 = normalize(float3(0.f, downpos.y - uppos.y, -2.f));
    
    float3 normal = normalize(cross(v1, v2));
    
    float3 tangent = cross(v1, normal);
    
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
        pos += mul(float4(input.pos, 1.f), g_vecBones[input.blendIndex[i] + g_pBone[0].g_iBoneMaxJoint * iInstId]) * fWeight[i];
        normal += mul(input.normal, (float3x3) g_vecBones[input.blendIndex[i] + g_pBone[0].g_iBoneMaxJoint * iInstId]) * fWeight[i];
        tangent += mul(input.tangent.xyz, (float3x3) g_vecBones[input.blendIndex[i] + g_pBone[0].g_iBoneMaxJoint * iInstId]) * fWeight[i];
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
    
    output.vDiffuseColor = input.diffuse;
    output.vSpecularColor = input.specular;
    output.vMaterialRoughness = input.roughness;
    output.fMaterialFraction = input.fraction;
    output.clip = output.pos;
    
    return output;
}

PSOut PS(VSOut input)
{
    PSOut output;

    // Emissive is routed through MRT4 (added in PS_Multi after lighting),
    // so it must NOT be baked into baseColor (would otherwise get attenuated
    // by NDotL/materialFraction).
    float3 baseColor = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    float3 specularRGB = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    // Optional Metalness texture (t9): converts a metalness-workflow
    // asset (single base color) into the engine's specular workflow.
    // metalness=0 keeps the supplied diffuse/specular pair as-is.
    // metalness=1 mutes diffuse and tints F0 with base color.
    //
    // F0 (= value3, the actual input to GetFresnel in PS_Multi) starts
    // from the material uniform g_vSpecularColor (dielectric F0, ~0.04
    // by default) and lerps toward the original (pre-metalness) base
    // color when the metalness texture says the surface is metallic.
    // We must capture baseColor BEFORE the `baseColor *= (1-metalness)`
    // step below, otherwise metals would tint F0 with black.
    float3 F0 = g_vSpecularColor.xyz;
    uint mW, mH;
    g_MetalnessTexture.GetDimensions(mW, mH);
    if (mW > 0)
    {
        float metalness = g_MetalnessTexture.Sample(g_sAnisotropic, input.uv).r;
        float3 metalBase = baseColor;                // original albedo
        F0 = lerp(F0, metalBase, metalness);
        specularRGB = lerp(specularRGB, baseColor, metalness);
        baseColor *= (1.0f - metalness);
    }

    // Optional AO texture (t8): pre-multiply into albedo when bound.
    uint aoW, aoH;
    g_AOTexture.GetDimensions(aoW, aoH);
    if (aoW > 0)
    {
        baseColor *= g_AOTexture.Sample(g_sAnisotropic, input.uv).r;
    }
    output.value0.xyz = baseColor;

    // 진단 1: VS가 PS로 넘긴 normal 자체
    //output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    // 진단 2: VS가 PS로 넘긴 tangent 자체
    //output.value1.xyz = input.tangent.xyz * 0.5f + 0.5f;
    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;

    output.value2.xyz = specularRGB;

    output.value3.xyz = F0;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    // Optional Roughness texture (t6): .r overrides uniform when bound.
    uint rW, rH;
    g_RoughnessTexture.GetDimensions(rW, rH);
    if (rW > 0)
    {
        float r = g_RoughnessTexture.Sample(g_sAnisotropic, input.uv).r;
        output.value0.w = r;
        output.value1.w = r;
    }
    else
    {
        output.value0.w = g_vMaterialRoughness.x;
        output.value1.w = g_vMaterialRoughness.y;
    }
    output.value2.w = g_fMaterialFraction;

    // UE MID-스타일 히트 플래시 + ShadingModelID 패킹 (value3.w).
    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

PSOut PS_NoSpecMap(VSOut input)
{
    PSOut output;

    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;

    output.value0.xyz = GetPaperBurnColor(g_vDiffuseColor * g_Texture.Sample(g_sAnisotropic, input.uv), input.uv).xyz;

    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

PSOut PS_NoNormalMap(VSOut input)
{
    PSOut output;

    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    output.value0.xyz = GetPaperBurnColor(g_vDiffuseColor * g_Texture.Sample(g_sAnisotropic, input.uv), input.uv).xyz;

    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

PSOut PS_NoSpecMapNoNormalMap(VSOut input)
{
    PSOut output;

    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    output.value0.xyz = GetPaperBurnColor(g_vDiffuseColor * g_Texture.Sample(g_sAnisotropic, input.uv), input.uv).xyz;

    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

PSOut PS_NoDiffuseNoSpecMapNoNormalMap(VSOut input)
{
    PSOut output;

    output.value1.xyz = normalize(input.normal) * 0.5f + 0.5f;

    output.value0.xyz = GetPaperBurnColor(g_vDiffuseColor, input.pos.xz).xyz;

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

PSOut PS_NoTexture(VSOut input)
{
    PSOut output;

    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;

    output.value0.xyz = GetPaperBurnColor(g_vDiffuseColor, input.uv).xyz;

    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

    return output;
}

PSOut PS_Terrain(VS_Terrain_Out input)
{
    PSOut output;

    float3 normal = normalize(input.normal);

    float3 tangent = normalize(input.tangent.xyz);

    float3 bitangent = cross(normal, tangent) * input.tangent.w;

    // Tile-index lookup. blend_uv may equal 1.0 exactly at the edge (VS uses
    // 1.f - pos.z/Width which hits 1.0 when pos.z == 0), so clamp to avoid
    // out-of-bounds reads into BlendTerrainTexture (StructuredBuffer<int>).
    int x_idx = clamp((int) (input.blend_uv.x * g_iTerrainWidth),  0, g_iTerrainWidth  - 1);
    int y_idx = clamp((int) (input.blend_uv.y * g_iTerrainHeight), 0, g_iTerrainHeight - 1);
    int iTileIndex = y_idx * g_iTerrainWidth + x_idx;

    // Tile type drives which slice of the Texture2DArray each sample reads.
    // Guard against garbage / negative values (texture-array layer indexing
    // wraps oddly otherwise).
    int iTileType = max(0, g_BlendTerrainTexture[iTileIndex]);

    float3 vNormal   = g_TerrainNormalTexture.Sample  (g_sAnisotropic, float3(input.uv, iTileType)).xyz;
    float3 vTexture  = g_TerrainTexture.Sample        (g_sAnisotropic, float3(input.uv, iTileType)).xyz;
    float3 vEmissive = g_TerrainEmissiveTexture.Sample(g_sAnisotropic, float3(input.uv, iTileType)).xyz;
    float3 vSpecular = g_TerrainSpecularTexture.Sample(g_sAnisotropic, float3(input.uv, iTileType)).xyz;

    float3 N = normalize(vNormal * 2.f - 1.f);

    output.value1.xyz = normalize(float3(
    tangent.x * N.x + bitangent.x * N.y + normal.x * N.z,
    tangent.y * N.x + bitangent.y * N.y + normal.y * N.z,
    tangent.z * N.x + bitangent.z * N.y + normal.z * N.z)) * 0.5f + 0.5f;

    //output.value1.xyz = normalize(normal.xyz) * 0.5f + 0.5f;

    output.value0.xyz = g_vDiffuseColor.xyz * vTexture;

    output.value0.w = g_vMaterialRoughness.x;
    output.value1.w = g_vMaterialRoughness.y;

    output.value2.xyz = vSpecular;

    output.value2.w = g_fMaterialFraction;

    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * vEmissive;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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
    
    GetLightDirAndColor(input.view, C, light);
    
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
    
    viewPos.z = ConvertZToLinearDepth(depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;
    
    float4 shadowpos = mul(float4(viewPos, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float4 fShadowAttr = 1.f;//g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z);

    float4 decal0 = g_DecalTexture0.Sample(g_sPoint, input.uv);
    float4 decal1 = g_DecalTexture1.Sample(g_sPoint, input.uv);
    float4 decal2 = g_DecalTexture2.Sample(g_sPoint, input.uv);
    float4 decal3 = g_DecalTexture3.Sample(g_sPoint, input.uv);
    
    float4 value0 = g_GBufferTexture0.Sample(g_sPoint, input.uv);
    
    float4 value1 = g_GBufferTexture1.Sample(g_sPoint, input.uv);
    
    float2 vMaterialRoughness = float2(value0.w, value1.w) * (1.f - decal2.w) + decal2.xy * decal2.w;
    
    float3 albedo = value0.xyz * (1.f - decal0.w) + decal0.xyz * decal0.w;
    
    float3 normal = normalize(((value1.xyz * (1.f - decal1.w) + decal1.xyz * decal1.w) - 0.5f) * 2.f);
    
    float4 value2 = g_GBufferTexture2.Sample(g_sPoint, input.uv);
    
    float4 value3 = g_GBufferTexture3.Sample(g_sPoint, input.uv);
    
    float3 vSpecColor = value3.xyz * (1.f - decal3.w) + decal3.xyz * decal3.w;
    
    float3 G = value2.xyz;
    
    float materialFraction = value2.w * (1.0 - decal2.w) + decal2.z * decal2.w;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    GetLightDirAndColor(viewPos, C, light);
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = max(dot(normal, hdir),0.0);
    
    float NDotL = max(dot(normal, light),0.0);
    
    float LDotH = max(dot(light, hdir),0.0);
    
    float NDotV = max(dot(normal, view), 0.0001);
    
    float3 reflect = 2.0 * (NDotV) * normal - view;
    
    float2 envUV = SphereMapping(normalize(reflect));
    
    float4 envColor = g_EnvironmentTexture.Sample(g_sAnisotropic, envUV);
    
    float3 P = normalize(hdir - NDotH * normal);
    
    float4 vFresnel = float4(GetFresnel(LDotH, vSpecColor), 1.0);
    
    //float HDotV = max(dot(hdir, view),0.0);
    
    //float3 Ks = GetF(HDotV, vSpecColor);
    //float3 kd = (1.0 - Ks) * (1.0 - materialFraction);
    
    //float3 lambert = albedo / 3.141592;
    
    float hdenominator = max((hdir.x * hdir.x + hdir.y * hdir.y), 0.000001);
    
    float4 vMicroFacet = GetMicrofacetDistribution(NDotH, hdir.x * hdir.x / hdenominator, vMaterialRoughness);
    
    float4 vGeometry = GetGeometricAttenuation(NDotH, NDotV, NDotL, LDotH);
    
    //float3 cookTorranceNumerator = vMicroFacet.xyz * vGeometry.xyz * GetF(HDotV, vSpecColor);
    //float cookTorranceDenominator = 4.0 * max(NDotV, 0.0) * max(NDotL, 0.0);
    //cookTorranceDenominator = max(cookTorranceDenominator, 0.000001);
    //float3 cookTorrance = cookTorranceNumerator / cookTorranceDenominator;
    
    //float3 BRDF = kd * lambert + cookTorrance;
    
    //float3 outgoingLight = BRDF * envColor.xyz * max(NDotL, 0.0);
    
    // UE-style Shading Model 분기: BasePass가 value3.w(0..1)로 ShadingModelID 패킹,
    // 여기서 디코드해 라이팅 모델을 선택. 환경(DefaultLit)·캐릭터(Toon)·UI(Unlit) 혼용.
    uint shadingId = (uint)round(value3.w * 255.0);

    float3 emissive = g_GBufferTexture4.Sample(g_sPoint, input.uv).xyz;

    float3 worldPixelPos = mul(float4(viewPos, 1.f), g_matInvView).xyz;
    float3 worldCamPos   = mul(float4(0.f, 0.f, 0.f, 1.f), g_matInvView).xyz;

    if (shadingId == SHADING_MODEL_UNLIT)
    {
        float3 unlitOut = albedo + emissive;
        unlitOut = ApplyFog(unlitOut, worldCamPos.y, worldPixelPos - worldCamPos);
        return float4(unlitOut, 1.f);
    }

    if (shadingId == SHADING_MODEL_TOON)
    {
        // 3단 셀쉐이딩 + 하드 림. 그림자는 NDotL 밴드와 곱해 셰도우 컷오프 유지.
        const float fBandCount = 3.0;
        float ndlRaw = saturate(dot(normal, light));
        float ndlToon = floor(ndlRaw * fBandCount + 0.0001) / fBandCount;
        // 림: 시야와 거의 직각인 픽셀에 하드 라인. 메탈 캐릭터에선 specColor를
        // 따라가도록 vSpecColor와 곱.
        float fRim = 1.0 - saturate(dot(normal, view));
        fRim = smoothstep(0.55, 0.85, fRim);
        float3 toonLit = albedo * C.rgb * ndlToon * fShadowAttr.x
                       + fRim * vSpecColor;
        toonLit += emissive;
        toonLit = ApplyFog(toonLit, worldCamPos.y, worldPixelPos - worldCamPos);
        return float4(toonLit, 1.f);
    }

    // SHADING_MODEL_DEFAULT_LIT — 기존 마이크로페이셋 BRDF.
    float4 finalColor = (materialFraction * C * float4(albedo, 1.f) * max(NDotL, 0.f)
    + (1.f - materialFraction) * saturate(C * envColor * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV)) * fShadowAttr;

    // Per-pixel emissive from MRT4 (written by BasePass PS variants).
    // Added after shadow attenuation so emissive isn't darkened by shadow;
    // fog still applies below.
    finalColor.xyz += emissive;

    // ApplyFog needs world-space data for both args. Pass camera world Y
    // (g_matInvView row 3 column = camera position) as eyePosY, and the
    // world-space camera→pixel vector as eyeToPixel.
    finalColor.xyz = ApplyFog(finalColor.xyz, worldCamPos.y, worldPixelPos - worldCamPos);

    return finalColor;
    
    //return saturate(C * vFresnel * vMicroFacet * vGeometry / 3.141592f / NDotV) * fShadowAttr;

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

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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

    output.value4.xyz = g_vEmissiveColor.xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    output.value0.xyz = ApplyHitFlash(output.value0.xyz);
    output.value3.w = EncodeShadingId();

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
    
    float fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z).r;
    
    float2 vMaterialRoughness = g_vMaterialRoughness;
    
    float4 albedo = g_Texture.Sample(g_sAnisotropic, input.uv) * g_vDiffuseColor + g_vEmissiveColor * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv);
    
    albedo = GetPaperBurnColor(albedo, input.uv);
    
    float3 normal = BumpMapping(input.normal, input.tangent, input.uv);
    
    float4 vSpecColor = g_SpecularTexture.Sample(g_sAnisotropic, input.uv) * g_vSpecularColor;
    
    float materialFraction = g_fMaterialFraction;
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    GetLightDirAndColor(input.view, C, light);
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    return BRDF(view, hdir, normal, light, albedo, vSpecColor, C, vMaterialRoughness, materialFraction, fShadowAttr);
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
    
    float fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z).r;
    
    float4 albedo = g_vDiffuseColor;
    
    albedo = GetPaperBurnColor(albedo, input.uv);
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    GetLightDirAndColor(input.view, C, light);
    
    float3 view = normalize(-input.view);
    
    float3 hdir = normalize(light + view);
    
    return BRDF(view, hdir, input.normal, light, albedo, g_vSpecularColor, C, g_vMaterialRoughness, g_fMaterialFraction, fShadowAttr) + g_vEmissiveColor;
}

float4 PS_AlphaNoUVNoShadow(VSOut input) : SV_Target
{
    float4 albedo = g_vDiffuseColor;
    
    albedo = GetPaperBurnColor(albedo, input.uv);
    
    float3 light = 0.f;
    
    float4 C = 0.f;
    
    GetLightDirAndColor(input.view, C, light);
    
    float3 view = normalize(-input.view);
    
    float3 hdir = normalize(light + view);
    
    return BRDF(view, hdir, input.normal, light, albedo, g_vSpecularColor, C, g_vMaterialRoughness, g_fMaterialFraction, 1.f) + g_vEmissiveColor;
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
    
    float fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z).r;

    float2 vMaterialRoughness = input.vMaterialRoughness;

    float4 albedo = g_Texture.Sample(g_sAnisotropic, input.uv) * input.vDiffuseColor;

    float3 normal = BumpMapping(input.normal, input.tangent, input.uv);

    float4 vSpecColor = g_SpecularTexture.Sample(g_sAnisotropic, input.uv) * input.vSpecularColor;

    float materialFraction = input.fMaterialFraction;

    float3 light = 0.f;

    float4 C = 0.f;

    GetLightDirAndColor(input.view, C, light);

    float3 view = normalize(-viewPos);

    float3 hdir = normalize(light + view);

    return BRDF(view, hdir, normal, light, albedo, vSpecColor, C, vMaterialRoughness, materialFraction, fShadowAttr)
        + g_vEmissiveColor * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv);
}

float4 PS_AlphaNoUVInst(VSInstOut input) : SV_Target
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
    
    float fShadowAttr = g_ShadowTexture.SampleCmp(g_sShadow, shadowpos.xy, shadowpos.z).r;
    
    float4 albedo = input.vDiffuseColor;

    float3 light = 0.f;

    float4 C = 0.f;

    GetLightDirAndColor(input.view, C, light);

    float3 view = normalize(-input.view);

    float3 hdir = normalize(light + view);

    return BRDF(view, hdir, input.normal, light, albedo, input.vSpecularColor, C, input.vMaterialRoughness, input.fMaterialFraction, fShadowAttr)
        + g_vEmissiveColor;
}
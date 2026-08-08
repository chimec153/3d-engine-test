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
    // by NDotL).
    float3 baseColor = g_vDiffuseColor.xyz * g_Texture.Sample(g_sAnisotropic, input.uv).xyz;
    float3 specularRGB = g_SpecularTexture.Sample(g_sAnisotropic, input.uv).xyz;

    // metallic-roughness 워크플로우. metallic은 텍스처(t9) 있으면 .r,
    // 없으면 머티리얼 유니폼 g_vMaterialRoughness.y. PS_Multi가 이 값으로
    //   F0      = lerp(value3(유전체 F0), albedo, metallic)
    //   diffuse = albedo * (1 - metallic)
    // 를 계산하므로 여기선 albedo를 깎거나 specular를 합성하지 않는다.
    float metallic = g_vMaterialRoughness.y;
    uint mW, mH;
    g_MetalnessTexture.GetDimensions(mW, mH);
    if (mW > 0)
    {
        metallic = g_MetalnessTexture.Sample(g_sAnisotropic, input.uv).r;
    }

    // Optional AO texture (t8): pre-multiply into albedo when bound.
    uint aoW, aoH;
    g_AOTexture.GetDimensions(aoW, aoH);
    if (aoW > 0)
    {
        baseColor *= g_AOTexture.Sample(g_sAnisotropic, input.uv).r;
    }
    output.value0.xyz = baseColor;

    output.value1.xyz = BumpMapping(input.normal, input.tangent, input.uv) * 0.5f + 0.5f;

    output.value2.xyz = specularRGB;

    // value3.xyz = 유전체 F0 베이스(~0.04). 디퓨드에서 metallic으로 albedo와 lerp.
    output.value3.xyz = g_vSpecularColor.xyz;

    output.value4.xyz = g_vEmissiveColor.xyz * g_EmissiveTexture.Sample(g_sAnisotropic, input.uv).xyz;
    output.value4.w = 1.f;

    // roughness: 텍스처(t6) 있으면 .r override, 없으면 유니폼.
    uint rW, rH;
    g_RoughnessTexture.GetDimensions(rW, rH);
    float roughness = (rW > 0) ? g_RoughnessTexture.Sample(g_sAnisotropic, input.uv).r : g_vMaterialRoughness.x;

    output.value0.w = roughness;   // 디퓨드가 읽는 roughness
    output.value1.w = metallic;    // 디퓨드가 읽는 metallic
    output.value2.w = 0.f;         // 구 materialFraction (metallic-roughness에선 미사용)

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

// 부드러운 그림자(PCF). 비교-샘플러(s3)는 탭당 하드웨어 2x2 PCF지만, 단일 탭이면
// penumbra가 ~1텍셀이라 0/1처럼 보인다. 3x3 탭을 텍셀 간격으로 평균내 가장자리를
// 넓힌다. kShadowSoftness로 폭 조절(키우면 더 부드럽고, 너무 키우면 누수/피터팬닝).
float SampleShadowPCF(float2 uv, float compareZ)
{
    uint w, h;
    g_ShadowTexture.GetDimensions(w, h);
    float2 texel = 1.0f / float2(max(w, 1u), max(h, 1u));
    const float kShadowSoftness = 1.5f;

    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            sum += g_ShadowTexture.SampleCmp(g_sShadow, uv + float2(x, y) * texel * kShadowSoftness, compareZ).r;
        }
    }
    return sum / 9.0f;
}

float4 PS_Multi(VSMultiOut input)   :   SV_TARGET
{
    float3 viewPos = 0.f;
    
    float depth = g_DepthTexture0.Sample(g_sPoint, input.uv).r;
    
    float2 pos = input.uv * 2.f - 1.f;
    
    pos.y = -pos.y;
    
    viewPos.z = ConvertZToLinearDepth(depth);
    viewPos.xy = pos * viewPos.z * g_vProjectValues.xy;

    // --- 노멀 오프셋 그림자 바이어스 (grazing 슬로프 아크네 = 벽 줄무늬 제거) ---
    // 셰이딩 점을 표면 노멀 방향으로 살짝 밀어 자기-그림자를 피한다. grazing(N·L
    // 작음, 벽)일수록 sin으로 더 밀고, 바닥(N·L≈1)은 거의 안 건드려 회귀 없음.
    // 오프셋은 뷰=월드 단위. 줄무늬가 남으면 kShadowNormalOffset를 키우고, 그림자가
    // 표면에서 떠 보이면(peter-panning) 줄여라. 라이팅용 viewPos는 그대로 두고
    // 그림자 조회에만 쓰는 별도 위치(viewPosShadow)를 만든다.
    const float kShadowNormalOffset = 0.12f;
    float3 nrmVS   = normalize((g_GBufferTexture1.Sample(g_sPoint, input.uv).xyz - 0.5f) * 2.f);
    float3 lightVS = normalize(-g_vLightDir);
    float  ndl     = saturate(dot(nrmVS, lightVS));
    float  slope   = sqrt(saturate(1.f - ndl * ndl));   // sin(각): grazing일수록 1
    float3 viewPosShadow = viewPos + nrmVS * (kShadowNormalOffset * slope);

    float4 shadowpos = mul(float4(viewPosShadow, 1.f), g_matCameraViewToLightClip);
    
    shadowpos.xyz /= shadowpos.w;
    
    shadowpos.xy += 1.f;
    
    shadowpos.xy *= 0.5f;
    
    shadowpos.y = 1.f - shadowpos.y;
    
    float fShadowAttr = SampleShadowPCF(shadowpos.xy, shadowpos.z);

    float4 decal0 = g_DecalTexture0.Sample(g_sPoint, input.uv);
    float4 decal1 = g_DecalTexture1.Sample(g_sPoint, input.uv);
    float4 decal2 = g_DecalTexture2.Sample(g_sPoint, input.uv);
    float4 decal3 = g_DecalTexture3.Sample(g_sPoint, input.uv);
    float4 decal4 = g_DecalTexture4.Sample(g_sPoint, input.uv);
    
    float4 value0 = g_GBufferTexture0.Sample(g_sPoint, input.uv);
    
    float4 value1 = g_GBufferTexture1.Sample(g_sPoint, input.uv);
    
    float2 vMaterialRoughness = float2(value0.w, value1.w) * (1.f - decal2.w) + decal2.xy * decal2.w;
    
    float3 albedo = value0.xyz * (1.f - decal0.w) + decal0.xyz * decal0.w;
    
    float3 normal = normalize(((value1.xyz * (1.f - decal1.w) + decal1.xyz * decal1.w) - 0.5f) * 2.f);
    
    float4 value2 = g_GBufferTexture2.Sample(g_sPoint, input.uv);
    
    float4 value3 = g_GBufferTexture3.Sample(g_sPoint, input.uv);
    
    float3 vSpecColor = value3.xyz * (1.f - decal3.w) + decal3.xyz * decal3.w;
    
    float3 G = value2.xyz;

    float3 light = 0.f;
    
    float4 C = 0.f;
    
    GetLightDirAndColor(viewPos, C, light);
    
    float3 view = normalize(-viewPos);
    
    float3 hdir = normalize(light + view);
    
    float NDotH = max(dot(normal, hdir),0.0);
    
    float NDotL = max(dot(normal, light),0.0);

    float NDotV = max(dot(normal, view), 0.0001);

    float3 reflect = 2.0 * (NDotV) * normal - view;
    
    float2 envUV = SphereMapping(normalize(reflect));
    
    float4 envColor = g_EnvironmentTexture.Sample(g_sAnisotropic, envUV);
    
    // UE-style Shading Model 분기: BasePass가 value3.w(0..1)로 ShadingModelID 패킹,
    // 여기서 디코드해 라이팅 모델을 선택. 환경(DefaultLit)·캐릭터(Toon)·UI(Unlit) 혼용.
    uint shadingId = (uint)round(value3.w * 255.0);

    float3 emissive = g_GBufferTexture4.Sample(g_sPoint, input.uv).xyz;
    // MRT4 (emissive) is R11G11B10_FLOAT — no alpha channel, so decal4.w samples
    // as 1.0 and a lerp here wipes emissive to decal4.xyz. Emissive is additive
    // light anyway, so add the decal's emissive on top; decal MRT4 clears to 0,
    // so this is a no-op until an emissive decal writes value4 (see PS_DECAL_RING).
    emissive += decal4.xyz; // composite decal emissive (e.g. glowing ring)

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
        float3 toonLit = albedo * C.rgb * ndlToon * fShadowAttr
                       + fRim * vSpecColor;
        toonLit += emissive;
        toonLit = ApplyFog(toonLit, worldCamPos.y, worldPixelPos - worldCamPos);
        return float4(toonLit, 1.f);
    }

    // SHADING_MODEL_DEFAULT_LIT — metallic-roughness Cook-Torrance (GGX/Schlick).
    // roughness에 최소값을 줘 GGX 분포가 델타로 치솟는 걸 막는다(매끈한 면의
    // 스페큘러 파이어플라이 방지).
    float roughness = max(vMaterialRoughness.x, 0.05);   // value0.w
    float metallic  = vMaterialRoughness.y;              // value1.w

    // F0: 유전체는 value3(~0.04), 금속은 albedo. 디퓨즈는 (1-metallic)로 감쇠.
    float3 F0 = lerp(vSpecColor, albedo, metallic);
    float VDotH = max(dot(view, hdir), 0.0);
    float3 Fdir = GetF(VDotH, F0);
    float Dggx = DistributionGGX(NDotH, roughness);
    float Gsm  = GeometrySmith(NDotV, NDotL, roughness);

    // 분모 floor를 키우고 스페큘러를 캡한다. grazing 각에서 스페큘러가 폭주하면
    // (파이어플라이) HDR.fx의 산술평균 자동노출이 치솟아 화면 전체가 검게 죽는다.
    // 옛 specular-workflow의 saturate() 역할을 HDR 친화적 상한으로 대체.
    float3 specDir = (Dggx * Gsm) * Fdir / max(4.0 * NDotV * NDotL, 0.0025);
    specDir = min(specDir, 16.0);
    float3 kd = (1.0 - Fdir) * (1.0 - metallic);
    float3 diffDir = kd * albedo / 3.141592;

    float3 direct = (diffDir + specDir) * C.rgb * NDotL * fShadowAttr;

    // --- 전역 앰비언트 fill ---
    // 직사광이 안 닿는 픽셀이 순흑이 되지 않도록 앰비언트를 더한다. 라이트 선택과
    // 무관한 전역값 g_vAmbient(rgb=색, a=세기)를 RenderManager가 b12로 업로드하고
    // 에디터 RenderManager 창에서 조절. 금속은 디퓨즈가 없어 (1-metallic)로 차단
    // (대신 아래 ambSpec로 반사). 주의: 라이트별 가산 패스라 도미넌트 라이트 1개
    // 가정(emissive와 동일) — 라이트가 많으면 세기를 낮춰 보정.
    float3 ambientDiffuse = g_vAmbient.rgb * g_vAmbient.a * albedo * (1.0 - metallic);

    // 환경 반사(스페큘러 IBL): 금속은 albedo로 틴트된 F0로 env를 반사, roughness가
    // 높을수록 약화. 유전체(F0~0.04)는 미미.
    float3 Famb = GetF(NDotV, F0);
    float3 ambSpec = min(envColor.xyz * Famb * (1.0 - roughness), 8.0);

    // emissive는 그림자/라이팅 영향 없이 가산, fog는 마지막에 적용.
    float3 lit = direct + ambientDiffuse + ambSpec + emissive;
    lit = ApplyFog(lit, worldCamPos.y, worldPixelPos - worldCamPos);

    return float4(lit, 1.f);
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

    float3 light = 0.f;

    float4 C = 0.f;

    GetLightDirAndColor(input.view, C, light);

    float3 view = normalize(-viewPos);

    float3 hdir = normalize(light + view);

    return BRDF(view, hdir, normal, light, albedo, vMaterialRoughness.x, vMaterialRoughness.y, g_vSpecularColor.xyz, C, fShadowAttr);
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
    
    return BRDF(view, hdir, input.normal, light, albedo, g_vMaterialRoughness.x, g_vMaterialRoughness.y, g_vSpecularColor.xyz, C, fShadowAttr) + g_vEmissiveColor;
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
    
    return BRDF(view, hdir, input.normal, light, albedo, g_vMaterialRoughness.x, g_vMaterialRoughness.y, g_vSpecularColor.xyz, C, 1.f) + g_vEmissiveColor;
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

    float3 light = 0.f;

    float4 C = 0.f;

    GetLightDirAndColor(input.view, C, light);

    float3 view = normalize(-viewPos);

    float3 hdir = normalize(light + view);

    return BRDF(view, hdir, normal, light, albedo, vMaterialRoughness.x, vMaterialRoughness.y, input.vSpecularColor.xyz, C, fShadowAttr)
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

    return BRDF(view, hdir, input.normal, light, albedo, input.vMaterialRoughness.x, input.vMaterialRoughness.y, input.vSpecularColor.xyz, C, fShadowAttr)
        + g_vEmissiveColor;
}
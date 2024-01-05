#define POINT_LIGHT 0
#define SPOT_LIGHT 1
#define DIRECTIONAL_LIGHT 2

struct VSIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float2 uv : Texcoord;
};

struct VSStandardIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float3 blendWeight : BLENDWEIGHT;
    float2 uv : Texcoord;
    float4 blendIndex : BLENDINDICES;
};

struct VSStandardInstIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float3 blendWeight : BLENDWEIGHT;
    float2 uv : Texcoord;
    float4 blendIndex : BLENDINDICES;
    matrix WVP : World;
    matrix WV : View;
    matrix matCameraViewToLightClip : LIGHTVP;
    float4 diffuse : Material0;
    float4 specular : Material1;
    float2 roughness : Material2;
    float fraction : Material3;
    matrix joint : JointSocket;
    int instID : Bone0;
    int parentJoint : Bone1;
    int parentJointCount : Bone2;
};

struct VSInstIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float2 uv : Texcoord;
    matrix matWVP : World;
    matrix matWV : WorldView;
    matrix matCameraViewToLightClip : LIGHTVP;
    float4 vDiffuseColor : Diffuse;
    float4 vSpecularColor : Specular;
    float2 vMaterialRoughness : Roughness;
    float fMaterialFraction : MaterialFraction;
};

struct VSDecalInstIn
{
    float4 tangent : Tangent;
    float3 pos : Position;
    float3 normal : Normal;
    float3 blendWeight : BLENDWEIGHT;
    float2 uv : Texcoord;
    float4 blendIndex : BLENDINDICES;
    matrix WVP : World;
    matrix matInvWorldView : InvWorldView;
    float4 vDiffuseColor : Diffuse;
    float4 vSpecularColor : Specular;
    float4 vEmissiveColor : Emissive;
    float2 vMaterialRoughness : Roughness;
    float fMaterialFraction : MaterialFraction;
    float fDecalFadeStart : DECAL;
    float fDecalFadeMax : DECAL1;
    float fDecalFadeTime : DECAL2;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv : Texcoord;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 light : LIGHT;
    float3 lightDir : LIGHTDIR;
    float3 view : VIEW;
    float4 clip : Position;
};

struct VS_Terrain_Out
{
    float4 pos : SV_Position;
    float2 uv : Texcoord1;
    float2 blend_uv : Texcoord2;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 light : LIGHT;
    float3 lightDir : LIGHTDIR;
    float3 view : VIEW;
};

struct VSMultiOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct VSInstOut
{
    float4 pos : SV_Position;
    float2 uv : Texcoord;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 light : LIGHT;
    float3 lightDir : LIGHTDIR;
    float3 view : VIEW;
    float4 vDiffuseColor : Diffuse;
    float4 vSpecularColor : Specular;
    float2 vMaterialRoughness : Material;
    float fMaterialFraction : MaterialFrac;
    float4 clip : Position;
};

struct PSOut
{
    float4 value0 : SV_TARGET0;
    float4 value1 : SV_TARGET1;
    float4 value2 : SV_TARGET2;
    float4 value3 : SV_TARGET3;
};

struct VS_PARTICLE_OUT
{
    float4 pos : SV_Position;
    uint instID : INST;
};

struct VS_DECAL_OUT
{
    float4 pos : SV_Position;
    float4 screenpos : POSITION;
    float3 localpos : POSITION2;
    float2 uv : TEXCOORD;
};

struct VS_DECAL_INST_OUT
{
    float4 pos : SV_Position;
    float4 screenpos : POSITION;
    float3 localpos : POSITION2;
    float2 uv : TEXCOORD;
    matrix invWV : INVWORLDVIEW;
    float4 diffuse : DIFFUSE;
    float4 specular : SPECULAR;
    float4 emissive : EMISSIVE;
    float2 roughness : ROUGHNESS;
    float fraction : FRACTION;
    float fadestart : DECAL0;
    float fademax : DECAL1;
    float fadetime : DECAL2;
};

cbuffer transform : register(b0)
{
    float4x4 g_matTransform;
    float4x4 g_matWorldView;
    float4x4 g_matLightWVP;
    float4x4 g_matJoint;
    float4x4 g_matWorld;
    float4x4 g_matView;
    float4x4 g_matProj;
    int g_iTransformJointSocket;
};

cbuffer color : register(b0)
{
    float4 g_vColor[6];
};

cbuffer downscale : register(b0)
{
    uint2 g_vDownScaleResolution;
    uint g_iDownScaleDomain;
    uint g_iDownScaleGroupSize;
    float g_fDownScaleAdaptation;
    float g_fBloomThreshold;
}

cbuffer UI : register(b5)
{
    float2 g_vUIStartUV;
    float2 g_vUIEndUV;
    float2 g_vUIStartPos;
    float2 g_vUISize;
}

cbuffer light : register(b1)
{
    float3 g_vLightPos;
    float g_fConstAttenuation;
    float4 g_vLightColor;
    float4 g_vLightAmbientColor;
    float3 g_vLightDir;
    float g_fLinearAttenuation;
    float g_fQuadraticAttenuation;
    int g_iLightType;
    float g_fLightIntensity;
    int pad;
};

cbuffer material : register(b2)
{
    float4 g_vDiffuseColor;
    float4 g_vAmbientColor;
    float4 g_vSpecularColor;
    float4 g_vEmissiveColor;
    float g_fMaterialSpecPower;
    float g_fMaterialFraction;
    float2 g_vMaterialRoughness;
    bool g_bMaterialUsePaperBurn;
};

cbuffer GBufferProject : register(b3)
{
    float4 g_vProjectValues;
    matrix g_matCameraViewToLightClip;
};

struct BoneInfo
{
    float g_fBoneTime;
    float g_fBoneMaxTime;
    int g_iBoneMaxFrame;
    int g_iBoneMaxJoint;
    float3 g_vBoneRootPos;
    int g_iBoneFrame;
    int g_iBoneNextFrame;
    float g_fBoneBlendMaxTime;
    float fSequenceTime;
    int g_iPad2;
};

cbuffer Bone : register(b4)
{
    BoneInfo g_pBone[2];
    int g_iBoneSequenceCount;
    float4 g_pBoneAdditiveBlend[64];
}

cbuffer Terrain : register(b5)
{
    int g_iTerrainWidth;
    int g_iTerrainHeight;
    int g_iTerrainBlendCount;
}

struct IkInfo
{
    int iIKJointIndex;
    float3 vIKJointPosition;
    int iIKRootIndex;
};

cbuffer IK : register(b6)
{
    IkInfo g_tIKInfo[256];
}

cbuffer Particle : register(b7)
{
    float4 g_vParticleStartColor;
    float4 g_vParticleEndColor;
    float3 g_vParticleVelocity;
    float g_fParticleMaxLifeTime;
    float3 g_vParticleAccelation;
    int g_iParticleMaxParticleCount;
    float3 g_vParticleMinimumPosition;
    int g_iParticleCreateCount;
    float2 g_vParticleStartSize;
    float2 g_vParticleEndSize;
    float3 g_vParticleMaximumPosition;
    int g_iParticleMaxFrame;
    float3 g_vParticleMaxVelocity;
    int g_iParticleFrameWidth;
    int g_iParticleFrameHeight;
}

cbuffer Global : register(b8)
{
    float g_fGlobalAccTime;
    float g_fGlobalDeltaTime;
    int g_iNoiseTextureWidth;
    int g_iNoiseTextureHeight;
}

cbuffer Decal : register(b9)
{
    float4x4 g_matInvWorldView;
    float g_fDecalFadeTime;
    float g_fDecalMaxFade;
    float g_fDecalFadeStart;
}

cbuffer PaperBurn : register(b10)
{
    float4  g_vPaperStartColor;
    float4  g_vPaperMidColor;
    float4  g_vPaperFinalColor;
    float   g_fPaperStartRate;
    float   g_fPaperMidRate;
    float   g_fPaperFinalRate;
    float   g_fPaperEndRate;
    float   g_fPaperTime;
    float   g_fPaperMaxTime;
}

cbuffer Fluid : register(b11)
{
    float g_fFluidc1;
    float g_fFluidc2;
    float g_fFluidc3;
    int g_iFluidWidth;
    float g_fFluidDist;
}

struct Transform
{
    float3 pos;
    float4 queternion;
    float3 scale;
};

struct Bone
{
    float time;
    int maxframe;
    int frame;
    int nextframe;
    int animationID;
    int rootpos;
};


struct Particle
{
    float3 pos;
    bool alive;
    float4 color;
    float3 speed;
    float age;
    float2 size;
    float maxage;
    int frame;
};

struct ParticleEmitter
{
    int count;
};

Texture2D g_Texture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SpecularTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);
Texture2D g_PaperBurnTexture : register(t4);
Texture2D g_EnvironmentTexture : register(t5);

Texture2D g_HDRTexture : register(t7);

Texture2D g_DepthTexture0 : register(t10);
Texture2D g_GBufferTexture0 : register(t11);
Texture2D g_GBufferTexture1 : register(t12);
Texture2D g_GBufferTexture2 : register(t13);
Texture2D g_GBufferTexture3 : register(t14);
Texture2D g_ShadowTexture : register(t15);
Texture2D g_HeightTexture : register(t16);
Texture2D g_NoiseTexture : register(t17);

Texture2DArray g_TerrainTexture : register(t20);
Texture2DArray g_TerrainNormalTexture : register(t21);
Texture2DArray g_TerrainSpecularTexture : register(t22);
Texture2DArray g_TerrainEmissiveTexture : register(t23);
StructuredBuffer<int> g_BlendTerrainTexture : register(t24);

Texture2D g_DecalTexture0 : register(t25);
Texture2D g_DecalTexture1 : register(t26);
Texture2D g_DecalTexture2 : register(t27);
Texture2D g_DecalTexture3 : register(t28);

StructuredBuffer<matrix> g_vecBones : register(t30);
StructuredBuffer<Transform> g_vecTransforms : register(t31);
StructuredBuffer<matrix> g_vecJointSockets : register(t32);
StructuredBuffer<Transform> g_vecBonePalette : register(t33);
StructuredBuffer<Bone> g_vecBoneBuffer : register(t34);
StructuredBuffer<int> g_vecJointHierarchyBuffer : register(t35);
StructuredBuffer<Transform> g_vecAdditiveTransforms : register(t36);

StructuredBuffer<float> g_vecPrevHeightField : register(t38);
StructuredBuffer<float> g_vecCurrentHeightField : register(t39);

StructuredBuffer<Particle> g_vecParticle : register(t40);

StructuredBuffer<float> g_AverageValues1D : register(t41);

RWStructuredBuffer<matrix> g_vecFinalBuffer : register(u0);
RWStructuredBuffer<matrix> g_vecPoseBuffer : register(u1);
RWStructuredBuffer<Particle> g_vecParticleInfo : register(u2);
RWStructuredBuffer<int> g_vecEmitter : register(u3);
RWStructuredBuffer<float> g_vecHeightField : register(u4);
RWStructuredBuffer<float> g_AverageLum : register(u5);
RWTexture2D<float4> g_HDRDownScaleTexture : register(u6);

sampler g_sPoint : register(s0);
sampler g_sLinear : register(s1);
sampler g_sAnisotropic : register(s2);
SamplerComparisonState g_sShadow : register(s3);

groupshared float SharedPositions[1024];

static const float4 LUM_FACTOR = float4(0.299, 0.587, 0.114, 0);

float4 GetLightAtt(float3 vPointToLight)
{
    float fDistFromLightToPoint = length(vPointToLight);
    
    return 1.f / (g_fConstAttenuation +
    fDistFromLightToPoint * g_fLinearAttenuation +
    g_fQuadraticAttenuation * fDistFromLightToPoint * fDistFromLightToPoint) * g_vLightColor;
}

float GetSpecular(float3 viewpos, float3 vPointToLight, float3 normal)
{
    float3 vHalfWay = normalize(-viewpos + vPointToLight);
    
    return pow(max(dot(vHalfWay, normal), 0.f), g_fMaterialSpecPower) * (dot(normal, vPointToLight) > 0);
}

float3 BumpMapping(float3 n, float4 t, float2 uv)
{
    float3 normal = normalize(n);
    
    float3 tangent = normalize(t.xyz);
    
    float3 bitangent = cross(normal, tangent) * t.w;
    
    float3 N = normalize(g_NormalTexture.Sample(g_sAnisotropic, uv).xyz * 2.f - 1.f);
    
    return normalize(
        float3(
            tangent.x * N.x + bitangent.x * N.y + normal.x * N.z,
            tangent.y * N.x + bitangent.y * N.y + normal.y * N.z,
            tangent.z * N.x + bitangent.z * N.y + normal.z * N.z
        )
    );
}

float3 BumpMapping(float3 n, float4 t, float3 bump)
{
    float3 normal = normalize(n);
    
    float3 tangent = normalize(t.xyz);
    
    float3 bitangent = cross(normal, tangent) * t.w;
    
    float3 N = normalize(bump * 2.f - 1.f);
    
    return normalize(
        float3(
            tangent.x * N.x + bitangent.x * N.y + normal.x * N.z,
            tangent.y * N.x + bitangent.y * N.y + normal.y * N.z,
            tangent.z * N.x + bitangent.z * N.y + normal.z * N.z
        )
    );
}

float4 GetPaperBurnColor(float4 color, float2 uv)
{
    return color;
    
    if(!g_bMaterialUsePaperBurn)
    {
        return color;
    }
    
    float fRate = g_fPaperTime / g_fPaperMaxTime * 3.0 - 1.f + g_PaperBurnTexture.Sample(g_sAnisotropic, uv).r; //  0.0 ~ 3.0
    
    if (fRate < g_fPaperStartRate)
    {
        return color;
    }
    
    else if(fRate < g_fPaperMidRate)
    {
        float fBlendRate = (fRate - g_fPaperStartRate) / (g_fPaperMidRate - g_fPaperStartRate);
        
        return color * (1.0 - fBlendRate) + g_vPaperStartColor * fBlendRate;
    }
    
    else if(fRate < g_fPaperFinalRate)
    {
        float fBlendRate = (fRate - g_fPaperMidRate) / (g_fPaperFinalRate - g_fPaperMidRate);
        
        return g_vPaperStartColor * (1.0 - fBlendRate) + g_vPaperMidColor * fBlendRate;
    }
    
    else if(fRate < g_fPaperEndRate)
    {
        float fBlendRate = (fRate - g_fPaperFinalRate) / (g_fPaperEndRate - g_fPaperFinalRate);
        
        return g_vPaperMidColor * (1.0 - fBlendRate) + g_vPaperFinalColor * fBlendRate;
    }
    
    clip(-1);

    return 0.0;
}

float ConvertZToLinearDepth(float depth)
{
    float linearDepth = g_vProjectValues.w / (g_vProjectValues.z - depth);

    return linearDepth;
}

float2 SphereDirectionToUV(float3 dir)
{    
    float2 uv = 0.f;
    
    uv.x = atan2(dir.x, dir.z) / 3.141592f / 2.f + 0.5f; // -pi ~ pi
    
    uv.y = asin(dir.y) / -3.141592 + 0.5f; // -pi/2 ~ pi/2
    
    return uv;
}

float2 SphereMapping(float3 r)
{
    float m = sqrt(r.x * r.x + r.y * r.y + pow(r.z + 1, 2));

    return float2(r.x / (2 * m) + 0.5, 1.0 - (r.y / (2 * m) + 0.5));
}

float4 GetFresnel(float LDotH, float4 vSpecColor)
{
    float4 rt = sqrt(vSpecColor);
    
    float4 etha = (1.f + rt) / (1.f - rt);
    
    float4 g = sqrt(etha * etha - 1.f + LDotH * LDotH);
    
    return (g - LDotH) * (g - LDotH) / (g + LDotH) / (g + LDotH) *
    ((LDotH * (g + LDotH) - 1.f) * (LDotH * (g + LDotH) - 1.f) / (LDotH * (g - LDotH) + 1.f) / (LDotH * (g - LDotH) + 1.f) + 1.f) / 2.f;
}

float3 GetF(float VDotH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1 - max(VDotH, 0.0), 5.0);
}

float3 GetFresnel(float LDotH, float3 vSpecColor)
{
    
    float3 rt = sqrt(vSpecColor);
    
    float3 etha = (1.f + rt) / (1.f - rt);
    
    float3 g = sqrt(etha * etha - 1.f + LDotH * LDotH);
    
    return pow(g - LDotH, 2.0) / max(pow(g + LDotH, 2.0), 0.000001) *
    (pow(LDotH * (g + LDotH) - 1.f, 2.0) / max(pow(LDotH * (g - LDotH) + 1.f, 2.0), 0.000001) + 1.f) / 2.f;
}

float4 GetMicrofacetDistribution(float NDotH, float2 vMaterialRoughness)
{
    return 1.f / 4.f / vMaterialRoughness.x / vMaterialRoughness.x * exp((NDotH * NDotH - 1.f) / (vMaterialRoughness.x * vMaterialRoughness.x * NDotH * NDotH));
}

float4 GetMicrofacetDistribution(float NDotH, float TDotPPow2, float2 vMaterialRoughness)
{
    float NDotHDenominator = max(pow(NDotH, 2.0), 0.000001);
    
    return 1.f / 4.f / max(vMaterialRoughness.x, 0.000001) / max(vMaterialRoughness.y, 0.000001) * exp((TDotPPow2 / max(pow(vMaterialRoughness.x, 2.0), 0.000001) + (1.f - TDotPPow2) / max(vMaterialRoughness.y, 2.0)) *
    (NDotH * NDotH - 1.f) / NDotHDenominator);
}

float4 GetGeometricAttenuation(float NDotH, float NDotV, float NDotL, float LDotH)
{
    return min(min(1.f, 2.f * NDotH * NDotV / max(LDotH, 0.000001)), 2.f * NDotH * NDotL / max(LDotH, 0.000001));
}

float4 BRDF(float3 view, float3 hdir, float3 normal, float3 light, float4 albedo, float4 vSpecColor, float4 C, float2 vMaterialRoughness, float materialFraction, float fShadowAttr)
{
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


void GetLightDirAndColor(in float3 view, out float4 C, out float3 light)
{
    if (g_iLightType == POINT_LIGHT)
    {
        C = GetLightAtt(g_vLightPos - view) * g_fLightIntensity;
        
        light = normalize(g_vLightPos - view);
    }
    else if (g_iLightType == SPOT_LIGHT)
    {
        light = normalize(g_vLightPos - view);
        
        C = GetLightAtt(g_vLightPos - view) * pow(max(dot(g_vLightDir, light), 0.f), g_fLightIntensity);
    }
    else if (g_iLightType == DIRECTIONAL_LIGHT)
    {
        light = normalize(-g_vLightDir);
        
        C = g_vLightColor * g_fLightIntensity;
    }
}
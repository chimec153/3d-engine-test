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

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv : Texcoord;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 light : LIGHT;
    float3 lightDir : LIGHTDIR;
    float3 view : VIEW;
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

cbuffer transform : register(b0)
{
    float4x4 g_matTransform;
    float4x4 g_matWorldView;
    float4x4 g_matLightWVP;
    float4x4 g_matJoint;
    float4x4 g_matWorld;
    float4x4 g_matView;
    float4x4 g_matProj;
    float4x4 g_matInvWorldView;
    int g_iTransformJointSocket;
};

cbuffer color : register(b0)
{
    float4 g_vColor[6];
};

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
    int g_iMaterialContainerIndex;
};

cbuffer GBufferProject : register(b3)
{
    float4 g_vProjectValues;
    matrix g_matCameraViewToLightClip;
};

cbuffer Bone : register(b4)
{
    float g_fBoneTime;
    float g_fBoneMaxTime;
    int g_iBoneMaxFrame;
    int g_iBoneMaxJoint;
    float3 g_vBoneRootPos;
    int g_iBoneFrame;
    int g_iBoneNextFrame;
    int g_iBoneInfoCount;
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
    matrix g_matInvView;
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
Texture2DArray g_BlendTerrainTexture : register(t24);

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

StructuredBuffer<Particle> g_vecParticle : register(t40);

RWStructuredBuffer<matrix> g_vecFinalBuffer : register(u0);
RWStructuredBuffer<matrix> g_vecPoseBuffer : register(u1);
RWStructuredBuffer<Particle> g_vecParticleInfo : register(u2);
RWStructuredBuffer<int> g_vecEmitter : register(u3);

sampler g_sPoint : register(s0);
sampler g_sLinear : register(s1);
sampler g_sAnisotropic : register(s2);
SamplerComparisonState g_sShadow : register(s3);

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

float3 BumpMapping(float3 n, float4 t, float2 uv, float3 bump)
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
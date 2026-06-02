#include "shared.hlsl"

float4 ShadowVS(VSStandardIn input) : SV_Position
{
    return mul(float4(input.pos, 1.f), g_matLightWVP);
}

float4 ShadowAnimVS(VSStandardIn input) : SV_Position
{
    float3 pos = 0.f;
    
    float4 vWeight = float4(input.blendWeight[0], input.blendWeight[1], input.blendWeight[2], 1.f - input.blendWeight[0] - input.blendWeight[1] - input.blendWeight[2]);
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        pos += vWeight[i] * mul(float4(input.pos, 1.f), g_vecBones[input.blendIndex[i]]).xyz;
    }
    
    if (g_iTransformJointSocket != -1)
    {
        return mul(float4(pos, 1.f), mul(g_matJoint, g_matLightWVP));
    }
    
    return mul(float4(pos, 1.f), g_matLightWVP);
}

float4 VSInstShadow(VSStandardInstIn input) : SV_Position
{
    return mul(float4(input.pos, 1.f), input.matCameraViewToLightClip);
}

float4 VS_SkinInstShadow(VSStandardInstIn input, uint iInstID : SV_InstanceID) : SV_Position
{
    float4 vWeight = float4(input.blendWeight.x, input.blendWeight.y, input.blendWeight.z, 1.f - input.blendWeight.x - input.blendWeight.y - input.blendWeight.z);
    
    float3 pos = 0.f;
    
    [unroll]
    for (int i = 0; i < 4;++i)
    {
        pos += vWeight[i] * mul(float4(input.pos, 1.f), g_vecBones[input.blendIndex[i] + iInstID * g_pBone[0].g_iBoneMaxJoint]).xyz;
    }
    
    if (input.parentJoint != -1)
    {
        return mul(float4(pos, 1.f), mul(input.joint, mul(g_vecJointSockets[input.parentJoint + input.instID * input.parentJointCount], input.matCameraViewToLightClip)));
    }
    
    return mul(float4(pos, 1.f), input.matCameraViewToLightClip);
}

float4 ShadowPS()    :   SV_TARGET
{
    return 1.f;
}
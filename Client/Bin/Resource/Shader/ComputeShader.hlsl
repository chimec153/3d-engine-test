#include "shared.hlsl"

float4 Slerp(float4 p0, float4 p1, float t)
{
    float Dot = dot(p0, p1);
    
    if (Dot < 0.f)
    {
        p1 = -p1;
        Dot = dot(p0, p1);
    }
    
    float theta = acos(Dot);
    
    float sin_theta = sqrt(1.f - Dot * Dot);
    
    float sin_theta_1_t = sin(theta * (1.f - t));
    
    float sin_theta_t = sin(theta * t);
    
    if (abs(sin_theta) <= 0.00001f)
    {
        return p0;
    }
    
    return (sin_theta_1_t * p0 + sin_theta_t * p1) / sin_theta;

}

[numthreads(32, 1, 1)]
void Sequence(uint3 DTid : SV_DispatchThreadID)
{
    float fRate = g_pBone[0].g_fBoneTime;
    
    uint iIndex = DTid.x * g_pBone[0].g_iBoneMaxFrame + g_pBone[0].g_iBoneFrame;
    uint iNextIndex = DTid.x * g_pBone[0].g_iBoneMaxFrame + g_pBone[0].g_iBoneNextFrame;
    
    float3 pos = (g_vecTransforms[iIndex].pos - g_pBone[0].g_vBoneRootPos) * (1.f - fRate) + (g_vecTransforms[iNextIndex].pos - g_pBone[0].g_vBoneRootPos) * fRate;
    float3 scale = g_vecTransforms[iIndex].scale * (1.f - fRate) + g_vecTransforms[iNextIndex].scale * fRate;
    float4 quaternion = Slerp(g_vecTransforms[iIndex].queternion, g_vecTransforms[iNextIndex].queternion, fRate);
    
    if (g_iBoneSequenceCount > 1)
    {
        float fRate2 = g_pBone[1].g_fBoneTime;
    
        uint iIndex2 = DTid.x * g_pBone[1].g_iBoneMaxFrame + g_pBone[1].g_iBoneFrame;
        uint iNextIndex2 = DTid.x * g_pBone[1].g_iBoneMaxFrame + g_pBone[1].g_iBoneNextFrame;
    
        float3 pos2 = (g_vecAdditiveTransforms[iIndex2].pos - g_pBone[1].g_vBoneRootPos) * (1.f - fRate2) + (g_vecAdditiveTransforms[iNextIndex2].pos - g_pBone[1].g_vBoneRootPos) * fRate2;
        float3 scale2 = g_vecAdditiveTransforms[iIndex2].scale * (1.f - fRate2) + g_vecAdditiveTransforms[iNextIndex2].scale * fRate2;
        float4 quaternion2 = Slerp(g_vecAdditiveTransforms[iIndex2].queternion, g_vecAdditiveTransforms[iNextIndex2].queternion, fRate2);
        
        float fBlend = 0.f;
        
        if (g_pBone[1].g_fBoneMaxTime - g_pBone[1].g_fBoneBlendMaxTime < g_pBone[1].fSequenceTime)
        {
            fBlend = (g_pBone[1].g_fBoneMaxTime - g_pBone[1].fSequenceTime) / g_pBone[1].g_fBoneBlendMaxTime;
            
            fBlend = g_pBoneAdditiveBlend[DTid.x / 4][DTid.x % 4] * clamp(fBlend, 0.f, 1.f);
        }
        else
        {
            fBlend = g_pBoneAdditiveBlend[DTid.x / 4][DTid.x % 4] * clamp(g_pBone[1].fSequenceTime / g_pBone[1].g_fBoneBlendMaxTime, 0.f, 1.f);
        }
        
        pos = pos * (1.f - fBlend) + pos2 * fBlend;
        scale = scale * (1.f - fBlend) + scale2 * fBlend;
        quaternion = Slerp(quaternion, quaternion2, fBlend);
    }
    
    matrix matPos =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        pos.x, pos.y, pos.z, 1.f
    };
    
    matrix matScale =
    {
        scale.x, 0.f, 0.f, 0.f,
        0.f, scale.y, 0.f, 0.f,
        0.f, 0.f, scale.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    
    matrix matRot = 
    {
        1.f - 2.f * quaternion.y * quaternion.y - 2.f * quaternion.z * quaternion.z, 2.f * (quaternion.x * quaternion.y + quaternion.z * quaternion.w), 2.f * (quaternion.x * quaternion.z - quaternion.y * quaternion.w), 0.f,
        2.f * (quaternion.x * quaternion.y - quaternion.z * quaternion.w), 1.f - 2.f * (quaternion.x * quaternion.x + quaternion.z * quaternion.z), 2.f * (quaternion.y * quaternion.z + quaternion.x * quaternion.w), 0.f,
        2.f * (quaternion.x * quaternion.z + quaternion.y * quaternion.w), 2.f * (quaternion.y * quaternion.z - quaternion.x * quaternion.w), 1.f - 2.f * (quaternion.x * quaternion.x + quaternion.y * quaternion.y), 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    
    matrix matIdentity =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    
    g_vecPoseBuffer[DTid.x] = mul(matScale, mul(matRot, matPos));
    
    g_vecFinalBuffer[DTid.x] = mul(g_vecBones[DTid.x], g_vecPoseBuffer[DTid.x]);
}

[numthreads(32, 32, 1)]
void SequenceInst(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= g_pBone[0].g_iBoneMaxJoint)
    {
        return;
    }
    
    float fRate = g_vecBoneBuffer[DTid.y].time;
    
    uint iRootIndex = g_vecBoneBuffer[DTid.y].frame + g_pBone[0].g_iBoneMaxFrame * g_pBone[0].g_iBoneMaxJoint * g_vecBoneBuffer[DTid.y].animationID;
    uint iRootNextIndex = g_vecBoneBuffer[DTid.y].nextframe + g_pBone[0].g_iBoneMaxFrame * g_pBone[0].g_iBoneMaxJoint * g_vecBoneBuffer[DTid.y].animationID;
    
    uint iIndex = DTid.x * g_pBone[0].g_iBoneMaxFrame + iRootIndex;
    uint iNextIndex = DTid.x * g_pBone[0].g_iBoneMaxFrame + iRootNextIndex;
    
    float3 pos = g_vecBonePalette[iIndex].pos * (1.f - fRate) + g_vecBonePalette[iNextIndex].pos * fRate;
    float3 scale = g_vecBonePalette[iIndex].scale * (1.f - fRate) + g_vecBonePalette[iNextIndex].scale * fRate;
    float4 quaternion = Slerp(g_vecBonePalette[iIndex].queternion, g_vecBonePalette[iNextIndex].queternion, fRate);
    
    if (g_vecBoneBuffer[DTid.y].rootpos == 1)
    {
        pos -= g_vecBonePalette[iRootIndex].pos * (1.f - fRate) + g_vecBonePalette[iRootNextIndex].pos * fRate;
    }
    
    matrix matPos =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        pos.x, pos.y, pos.z, 1.f
    };
    
    matrix matScale =
    {
        scale.x, 0.f, 0.f, 0.f,
        0.f, scale.y, 0.f, 0.f,
        0.f, 0.f, scale.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    
    matrix matRot =
    {
        1.f - 2.f * quaternion.y * quaternion.y - 2.f * quaternion.z * quaternion.z, 2.f * (quaternion.x * quaternion.y + quaternion.z * quaternion.w), 2.f * (quaternion.x * quaternion.z - quaternion.y * quaternion.w), 0.f,
        2.f * (quaternion.x * quaternion.y - quaternion.z * quaternion.w), 1.f - 2.f * (quaternion.x * quaternion.x + quaternion.z * quaternion.z), 2.f * (quaternion.y * quaternion.z + quaternion.x * quaternion.w), 0.f,
        2.f * (quaternion.x * quaternion.z + quaternion.y * quaternion.w), 2.f * (quaternion.y * quaternion.z - quaternion.x * quaternion.w), 1.f - 2.f * (quaternion.x * quaternion.x + quaternion.y * quaternion.y), 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    
    g_vecFinalBuffer[DTid.x + g_pBone[0].g_iBoneMaxJoint * DTid.y] = mul(g_vecBones[DTid.x], mul(matScale, mul(matRot, matPos)));
}

[numthreads(32, 1, 1)]
void PostProcess(uint3 DTid : SV_DispatchThreadID)
{
    g_vecFinalBuffer[DTid.x] = g_vecBones[DTid.x];
    
    //float3 pPos[256];
    
    //for (int i = 0; i < g_iBoneMaxJoint; ++i)
    //{
    //    pPos[i] = g_vecPoseBuffer[i][3].xyz;
    //}
    
    //int iCurrentIndex = g_iIKJointIndex;
    
    //int iParentIndex = g_vecJointHierarchyBuffer[g_iIKJointIndex];
    
    //float3 vPrevPos = g_vIKJointPosition;
    
    //while (iParentIndex != -1)
    //{
    //    float3 vPos = pPos[iParentIndex];
    
    //    float fDistance = length(vPos - pPos[iCurrentIndex]);
        
    //    pPos[iCurrentIndex] = vPrevPos;
        
    //    g_vecFinalBuffer[iCurrentIndex][3].xyz = vPrevPos;
    
    //    vPrevPos = normalize(vPos - vPrevPos) * fDistance + vPrevPos;
        
    //    iCurrentIndex = iParentIndex;
        
    //    iParentIndex = g_vecJointHierarchyBuffer[iParentIndex];
    //}
}

[numthreads(32,32,1)]
void CS_FLUID(uint3 iDispatchThreadID   :   SV_DispatchThreadID)
{
    if (g_iFluidWidth <= iDispatchThreadID.x)
    {
        return;
    }
    else if (iDispatchThreadID.x == 0 || iDispatchThreadID.x == g_iFluidWidth - 1)
    {
        int index = iDispatchThreadID.x + iDispatchThreadID.y * g_iFluidWidth;
        
        g_vecHeightField[index] = 0.0;
        
        return;
    }
    
    int index = iDispatchThreadID.x + iDispatchThreadID.y * g_iFluidWidth;
    
    int indexleft = iDispatchThreadID.x - 1 + iDispatchThreadID.y * g_iFluidWidth;
    
    int indexright = iDispatchThreadID.x + 1 + iDispatchThreadID.y * g_iFluidWidth;
    
    int indexup = iDispatchThreadID.x + (iDispatchThreadID.y + 1) * g_iFluidWidth;
    
    int indexdown = iDispatchThreadID.x + (iDispatchThreadID.y - 1) * g_iFluidWidth;
    
    g_vecHeightField[index] = g_fFluidc1 * g_vecCurrentHeightField[index] + g_fFluidc2 * g_vecPrevHeightField[index] 
    + g_fFluidc3 * (g_vecCurrentHeightField[indexleft] + g_vecCurrentHeightField[indexright] + g_vecCurrentHeightField[indexup] + g_vecCurrentHeightField[indexdown]);
}
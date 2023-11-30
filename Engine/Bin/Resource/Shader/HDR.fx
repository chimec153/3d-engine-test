#include "shared.hlsl"

float DownScale4x4(uint2 curPixel, uint groupThreadId)
{
    float avgLum = 0.0;
    
    if(curPixel.y < g_vDownScaleResolution.y)
    {
        int3 nFullResPos = int3(curPixel * 4, 0);
        float4 downScaled = float4(0.0, 0.0, 0.0, 0.0);
        
        [unroll]
        for (int i = 0; i < 4;++i)
        {
            [unroll]
            for (int j = 0; j < 4;++j)
            {
                downScaled += g_HDRTexture.Load(nFullResPos, int2(j, i));
            }
        }

        downScaled /= 16.0;
        
        avgLum = dot(downScaled, LUM_FACTOR);
        
        g_HDRDownScaleTexture[curPixel.xy] = downScaled;

        SharedPositions[groupThreadId] = avgLum;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    return avgLum;
}

float DownScale1024to4(uint dispatchThreadId, uint groupThreadId, float avgLum)
{
    [unroll]
    for (uint groupSize = 4, step1 = 1, step2 = 2, step3 = 3;
        groupSize < 1024;
        groupSize *= 4, step1 *= 4, step2 *= 4, step3 *= 4)
    {
        if(groupThreadId % groupSize == 0)
        {
            float stepAvgLum = avgLum;
            stepAvgLum += dispatchThreadId + step1 < g_iDownScaleDomain ?
                SharedPositions[groupThreadId + step1] : avgLum;
            stepAvgLum += dispatchThreadId + step2 < g_iDownScaleDomain ?
                SharedPositions[groupThreadId + step2] : avgLum;
            stepAvgLum += dispatchThreadId + step3 < g_iDownScaleDomain ?
                SharedPositions[groupThreadId + step3] : avgLum;
            
            avgLum = stepAvgLum;
            SharedPositions[groupThreadId] = stepAvgLum;
        }
        
        GroupMemoryBarrierWithGroupSync();
    }
    
    return avgLum;
}

void DownScale4to1(uint dispatchThreadId, uint groupThreadId, uint groupId, float avgLum)
{
    if(groupThreadId == 0)
    {
        float fFinalAvgLum = avgLum;
        
        fFinalAvgLum += dispatchThreadId + 256 < g_iDownScaleDomain ?
            SharedPositions[groupThreadId + 256] : avgLum;
        fFinalAvgLum += dispatchThreadId + 512 < g_iDownScaleDomain ?
            SharedPositions[groupThreadId + 512] : avgLum;
        fFinalAvgLum += dispatchThreadId + 768 < g_iDownScaleDomain ?
            SharedPositions[groupThreadId + 768] : avgLum;
        fFinalAvgLum /= 1024.0;

        g_AverageLum[groupId] = fFinalAvgLum;
    }
}

[numthreads(1024, 1, 1)]
void DownScaleFirstPass(uint3 groupId : SV_GroupID,
    uint3 dispatchThreadId : SV_DispatchThreadID,
    uint3 groupThreadId : SV_GroupThreadID)
{
    uint2 curPixel = uint2(dispatchThreadId.x % g_vDownScaleResolution.x, dispatchThreadId.x / g_vDownScaleResolution.x);
    
    float avgLum = DownScale4x4(curPixel, groupThreadId.x);
    
    avgLum = DownScale1024to4(dispatchThreadId.x, groupThreadId.x, avgLum);
    
    DownScale4to1(dispatchThreadId.x, groupThreadId.x, groupId.x, avgLum);
}

#define MAX_GROUPS 64

groupshared float SharedAvgFinal[MAX_GROUPS];

StructuredBuffer<float> PrevAvgLum : register(t2);

[numthreads(MAX_GROUPS, 1, 1)]
void DownScaleSecondPass(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float avgLum = 0.0;
    if(dispatchThreadId.x < g_iDownScaleGroupSize)
    {
        avgLum = g_AverageValues1D[dispatchThreadId.x];
    }
    
    SharedAvgFinal[dispatchThreadId.x] = avgLum;
    
    GroupMemoryBarrierWithGroupSync();
    
    if(dispatchThreadId.x % 4 == 0)
    {
        float stepAvgLum = avgLum;
        stepAvgLum += dispatchThreadId.x + 1 < g_iDownScaleGroupSize ?
            SharedAvgFinal[dispatchThreadId.x + 1] : avgLum;
        stepAvgLum += dispatchThreadId.x + 2 < g_iDownScaleGroupSize ?
            SharedAvgFinal[dispatchThreadId.x + 2] : avgLum;
        stepAvgLum += dispatchThreadId.x + 3 < g_iDownScaleGroupSize ?
            SharedAvgFinal[dispatchThreadId.x + 3] : avgLum;
        
        avgLum = stepAvgLum;
        SharedAvgFinal[dispatchThreadId.x] = stepAvgLum;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if(dispatchThreadId.x == 0)
    {
        float fFinalLumValue = avgLum;
        fFinalLumValue += dispatchThreadId.x + 16 < g_iDownScaleGroupSize ?
        SharedAvgFinal[dispatchThreadId.x + 16] : avgLum;
        fFinalLumValue += dispatchThreadId.x + 32 < g_iDownScaleGroupSize ?
        SharedAvgFinal[dispatchThreadId.x + 32] : avgLum;
        fFinalLumValue += dispatchThreadId.x + 48 < g_iDownScaleGroupSize ?
        SharedAvgFinal[dispatchThreadId.x + 48] : avgLum;
        fFinalLumValue /= 64.0;
        
        float fAdaptedAverageLum = lerp(PrevAvgLum[0], fFinalLumValue, g_fDownScaleAdaptation);
        
        g_AverageLum[0] = max(fAdaptedAverageLum, 0.0001);
    }
}

static const float2 arrBasePos[4] =
{
    float2(-1.0, 1.0),
    float2(1.0, 1.0),
    float2(-1.0, -1.0),
    float2(1.0, -1.0),
};

static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
};

struct VS_HDR_OUTPUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_HDR_OUTPUT FullScreenQuadVS(uint VertexID : SV_VertexID)
{
    VS_HDR_OUTPUT output;
    
    output.pos = float4(arrBasePos[VertexID].xy, 0.0, 1.0);
    output.uv = arrUV[VertexID].xy;
    
    return output;
}

Texture2D<float4> HDRTexture : register(t0);
StructuredBuffer<float> AvgLum : register(t1);
Texture2D<float4> HDRDownScaleTexture : register(t2);
Texture2D<float4> BloomTexture : register(t3);

RWTexture2D<float4> Bloom : register(u0);

cbuffer FinalPassConstants : register(b0)
{
    float MiddleGray : packoffset(c0);
    float LumWhiteSqr : packoffset(c0.y);
    float BloomScale : packoffset(c0.z);
    float DOFFarValueX : packoffset(c0.w);
    float DOFFarValueY : packoffset(c1);
}

float3 ToneMapping(float3 HDRColor)
{
    float LScale = dot(HDRColor, LUM_FACTOR.xyz);
    LScale *= MiddleGray / AvgLum[0];
    LScale = (LScale + LScale * LScale / LumWhiteSqr) / (1.0 + LScale);
    
    return HDRColor * LScale;
}

float3 DistanceDOF(float3 colorFocus, float3 colorBlurred, float depth)
{
    float blurFactor = saturate((depth - DOFFarValueX) * DOFFarValueY);
    
    return lerp(colorFocus, colorBlurred, blurFactor);
}

float4 FinalPassPS(VS_HDR_OUTPUT input) :   SV_TARGET
{
    float3 color = HDRTexture.Sample(g_sPoint, input.uv.xy).xyz;
    
    float depth = g_DepthTexture0.Sample(g_sPoint, input.uv.xy);
    
    if(depth < 1.0)
    {
        depth = ConvertZToLinearDepth(depth);
        
        float3 colorBlurred = HDRDownScaleTexture.Sample(g_sLinear, input.uv.xy).xyz;

        color = DistanceDOF(color, colorBlurred, depth);
    }
    
    color += BloomScale * BloomTexture.Sample(g_sLinear, input.uv.xy).xyz;
    
    color = ToneMapping(color);
    
    return float4(color, 1.0);
}

[numthreads(1024, 1, 1)]
void BrightPass(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 CurPixel = uint2(dispatchThreadId.x % g_vDownScaleResolution.x, dispatchThreadId.x / g_vDownScaleResolution.x);
    
    if(CurPixel.y < g_vDownScaleResolution.y)
    {
        float4 color = HDRDownScaleTexture.Load(int3(CurPixel, 0));
        
        float Lum = dot(color, LUM_FACTOR);
        float avgLum = AvgLum[0];
        
        float colorScale = saturate(Lum - avgLum * g_fBloomThreshold);
        
        Bloom[CurPixel.xy] = color * colorScale;
    }
}

Texture2D<float4> Input : register(t0);
RWTexture2D<float4> Output : register(u0);

static const float SampleWeights[13] =
{
    0.002216,
    0.008764,
    0.026995,
    0.064759,
    0.120985,
    0.176033,
    0.199471,
    0.176033,
    0.120985,
    0.064759,
    0.026995,
    0.008764,
    0.002216
};

#define kernelhalf  6
#define groupthreads 128

groupshared float4 SharedInput[groupthreads];

[numthreads(groupthreads, 1, 1)]
void VerticalFilter(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
    int2 coord = int2(Gid.x, GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.y);
    
    coord = clamp(coord, int2(0, 0), int2(g_vDownScaleResolution.x - 1, g_vDownScaleResolution.y - 1));
    SharedInput[GI] = Input.Load(int3(coord, 0));
    
    GroupMemoryBarrierWithGroupSync();
    
    if (GI >= kernelhalf && GI < (groupthreads - kernelhalf) &&
        ((GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.y) < g_vDownScaleResolution.y))
    {
        float4 vOut = 0;
        
        [unroll]
        for (int i = -kernelhalf; i <= kernelhalf;++i)
        {
            vOut += SharedInput[GI + i] * SampleWeights[i + kernelhalf];
        }

        Output[coord] = float4(vOut.rgb, 1.0f);
    }
}

[numthreads(groupthreads, 1, 1)]
void HorizonFilter(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
    int2 coord = int2(GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.x, Gid.y);

    coord = clamp(coord, int2(0, 0), int2(g_vDownScaleResolution.x - 1, g_vDownScaleResolution.y - 1));
    
    SharedInput[GI] = Input.Load(int3(coord, 0));
    
    GroupMemoryBarrierWithGroupSync();

    if (GI >= kernelhalf && GI < (groupthreads - kernelhalf) &&
        ((Gid.x * (groupthreads - 2 * kernelhalf) + GI - kernelhalf)< g_vDownScaleResolution.x))
    {
        float4 vOut = 0;
        
        [unroll]
        for(int i = -kernelhalf;i<=kernelhalf; ++i)
        {
            vOut += SharedInput[GI + i] * SampleWeights[i + kernelhalf];
        }
        
        Output[coord] = float4(vOut.rgb, 1.0f);
    }
}
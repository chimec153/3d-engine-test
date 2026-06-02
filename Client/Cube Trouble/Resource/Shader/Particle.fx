#include "shared.hlsl"

static const float g_pGaussianFilter[5][5] =
{
    1 / 273.f, 4 / 273.f, 7 / 273.f, 4 / 273.f, 1 / 273.f,
  4 / 273.f, 16 / 273.f, 26 / 273.f, 16 / 273.f, 4 / 273.f,
  7 / 273.f, 26 / 273.f, 41 / 273.f, 26 / 273.f, 7 / 273.f,
  4 / 273.f, 16 / 273.f, 26 / 273.f, 16 / 273.f, 4 / 273.f,
  1 / 273.f, 4 / 273.f, 7 / 273.f, 4 / 273.f, 1 / 273.f
};

float Random(float fSeed, float fSeed2)
{
    float2 uv = float2(cos(fSeed), sin(fSeed2));
    
    float value = 0;
    
    for (int i = -2; i < 3; ++i)
    {
        for (int j = -2; j < 3; ++j)
        {
            float u = i / (float) g_iNoiseTextureWidth;
            float v = j / (float) g_iNoiseTextureHeight;
            
            value += g_NoiseTexture.SampleLevel(g_sPoint, uv + float2(u, v), 0.f).x * g_pGaussianFilter[i + 2][j + 2];
        }
    }

    return value;
}

[numthreads(64, 1, 1)]
void CS_PARTICLE(uint3 iDispatchThreadID : SV_DispatchThreadID, uint3 iGroupID : SV_GroupID)
{
    if (!g_vecParticleInfo[iDispatchThreadID.x].alive)
    {
        if (g_vecEmitter[iGroupID.x] > 0)
        {            
            int iOriginValue = 0;
            
            InterlockedCompareExchange(g_vecEmitter[iGroupID.x], g_vecEmitter[iGroupID.x], g_vecEmitter[iGroupID.x] - 1, iOriginValue);
            
            if (iOriginValue == g_vecEmitter[iGroupID.x] + 1)
            {
                float fSeed = g_fGlobalAccTime;
                
                fSeed -= (int) (fSeed / (float)g_iNoiseTextureWidth) * g_iNoiseTextureWidth;
                
                fSeed *= iDispatchThreadID.x;
                
                float3 vRandom = float3
                (
                    Random(fSeed, fSeed * g_fGlobalDeltaTime * 100.f),
                    Random(fSeed * 10.f, fSeed * g_fGlobalDeltaTime * 10.f),
                    Random(fSeed * 100.f, fSeed * g_fGlobalDeltaTime * 2.f)
                );
                
                g_vecParticleInfo[iDispatchThreadID.x].alive = true;

                g_vecParticleInfo[iDispatchThreadID.x].age = 0.f;
                g_vecParticleInfo[iDispatchThreadID.x].maxage = g_fParticleMaxLifeTime * (1.f + Random(fSeed * 10.f, fSeed * g_fGlobalDeltaTime) * 0.2f);
                g_vecParticleInfo[iDispatchThreadID.x].pos = g_vParticleMinimumPosition * (1.f - vRandom) + g_vParticleMaximumPosition * vRandom + g_matWorld[3].xyz;
                g_vecParticleInfo[iDispatchThreadID.x].size = g_vParticleStartSize;
                
                float3 vSpeed = g_vParticleVelocity * (1.f - vRandom) + g_vParticleMaxVelocity * vRandom;
                if(length(vSpeed) == 0)
                {
                    g_vecParticleInfo[iDispatchThreadID.x].speed = float3(0.f, 0.f, 0.f);
                }
                else
                {
                    g_vecParticleInfo[iDispatchThreadID.x].speed = normalize(vSpeed);
                }
                g_vecParticleInfo[iDispatchThreadID.x].color = g_vParticleStartColor;

            }
        }
    }
    else
    {
        g_vecParticleInfo[iDispatchThreadID.x].age += g_fGlobalDeltaTime;

        if (g_vecParticleInfo[iDispatchThreadID.x].maxage < g_vecParticleInfo[iDispatchThreadID.x].age)
        {
            g_vecParticleInfo[iDispatchThreadID.x].alive = false;
            return;
        }

        float fRatio = g_vecParticleInfo[iDispatchThreadID.x].age / g_fParticleMaxLifeTime;
        
        g_vecParticleInfo[iDispatchThreadID.x].pos += g_vecParticleInfo[iDispatchThreadID.x].speed * g_fGlobalDeltaTime;
        g_vecParticleInfo[iDispatchThreadID.x].speed += g_vParticleAccelation * g_fGlobalDeltaTime;
        g_vecParticleInfo[iDispatchThreadID.x].color = (1.0f - fRatio) * g_vParticleStartColor + fRatio * g_vParticleEndColor;
        g_vecParticleInfo[iDispatchThreadID.x].size = (1.0f - fRatio) * g_vParticleStartSize + fRatio * g_vParticleEndSize;
        g_vecParticleInfo[iDispatchThreadID.x].frame = (int) (g_vecParticleInfo[iDispatchThreadID.x].age / g_vecParticleInfo[iDispatchThreadID.x].maxage * g_iParticleMaxFrame);

    }
}

VS_PARTICLE_OUT VS_PARTICLE(in unsigned int iInstID : SV_InstanceID)
{
    VS_PARTICLE_OUT output = (VS_PARTICLE_OUT) 0;
    
    output.instID = iInstID;
    
    return output;
}

[maxvertexcount(6)]
void GS_PARTICLE(point VS_PARTICLE_OUT p[1], inout TriangleStream<VSOut> OutputStream)
{
    if (!g_vecParticle[p[0].instID].alive)
    {
        return;
    }

    // Transform particle world position to view space ONCE up front. The
    // billboard quad's per-vertex offset is applied in view space and then
    // projected — sharing one view-space anchor keeps the quad coplanar
    // with the camera plane and lets us cull degenerate billboards.
    const float4 vViewPos = mul(float4(g_vecParticle[p[0].instID].pos, 1.f), g_matView);

    // Near-plane / behind-camera cull. Without this, a particle that
    // crosses (or starts behind) the camera near plane projects with a
    // tiny / negative w, blowing the billboard up into a screen-spanning
    // primitive — observed as a part of the screen turning solid white
    // when a particle reaches certain world coordinates. 0.1u matches the
    // engine's default near-plane budget; nudge higher if floaters bleed
    // through.
    if (vViewPos.z < 0.1f)
    {
        return;
    }

    VSOut _point[4] = (VSOut[4]) 0;

    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            _point[i + j * 2].pos = vViewPos;

            _point[i + j * 2].pos.x += (i * 2 - 1) * g_vecParticle[p[0].instID].size.x / 2.f;
            _point[i + j * 2].pos.y -= (j * 2 - 1) * g_vecParticle[p[0].instID].size.y / 2.f;

            _point[i + j * 2].pos = mul(_point[i + j * 2].pos, g_matProj);

            _point[i + j * 2].tangent = g_vecParticle[p[0].instID].color;

            _point[i + j * 2].uv.x = (g_vecParticle[p[0].instID].frame % g_iParticleFrameWidth + i) / (float) g_iParticleFrameWidth;
            _point[i + j * 2].uv.y = (g_vecParticle[p[0].instID].frame / g_iParticleFrameHeight + j) / (float) g_iParticleFrameHeight;
        }
    }
            
    OutputStream.Append(_point[0]);
    OutputStream.Append(_point[1]);
    OutputStream.Append(_point[2]);
    
    OutputStream.Append(_point[1]);
    OutputStream.Append(_point[3]);
    OutputStream.Append(_point[2]);
    
    OutputStream.RestartStrip();
}

float4 PS_PARTICLE(VSOut input) :   SV_Target
{
    return input.tangent * g_Texture.Sample(g_sAnisotropic, input.uv);

}

RWTexture2D<float4> g_BlurTexture : register(u0);

[numthreads(1024, 1, 1)]
void Blur(uint3 iDispatchThreadId : SV_DispatchThreadID)
{    
    int iPixelIndex = iDispatchThreadId.x + iDispatchThreadId.y * g_vDownScaleResolution.x * 4;
    
    if (iPixelIndex >= g_vDownScaleResolution.x * g_vDownScaleResolution.y * 16)
    {
        return;
    }
    
    int2 uv = int2(iPixelIndex % (g_vDownScaleResolution.x * 4), iPixelIndex / (g_vDownScaleResolution.x * 4));
    
    float4 color = 0.f;
    
    for (int i = -2; i <= 2;++i)
    {
        for (int j = -2; j <= 2;++j)
        {
            if(uv.x + i < 0 || uv.x + i >= g_vDownScaleResolution.x * 4 ||
                uv.y + j < 0 || uv.y + j >= g_vDownScaleResolution.y * 4)
            {
                continue;
            }
            
            color += g_Texture.Load(int3(uv + int2(i, j), 0)) * g_pGaussianFilter[i + 2][j + 2];

        }
    }
    
    g_BlurTexture[uv] = color;
}

const static float4 g_vPosition[4] =
{
    float4(-1.f, 1.f, 0.f, 1.f),
    float4(1.f, 1.f, 0.f, 1.f),
    float4(-1.f, -1.f, 0.f, 1.f),
    float4(1.f, -1.f, 0.f, 1.f)
};

const static float2 g_vUV[4] =
{
    float2(0.f, 0.f),
    float2(1.f, 0.f),
    float2(0.f, 1.f),
    float2(1.f, 1.f)
};

struct VS_OUT_NULL
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_OUT_NULL NullVS(uint iVertexID : SV_VertexID)
{
    VS_OUT_NULL output = (VS_OUT_NULL)0;
    
    output.pos = g_vPosition[iVertexID];
    output.uv = g_vUV[iVertexID];

    return output;
}

float4 NullPS(VS_OUT_NULL input)    :   SV_Target
{
    return g_Texture.Sample(g_sPoint, input.uv);
}
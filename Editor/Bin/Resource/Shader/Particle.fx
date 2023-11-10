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
void CS_PARTICLE(uint3 iDispatchThreadID : SV_DispatchThreadID)
{
    if (!g_vecParticleInfo[iDispatchThreadID.x].alive)
    {
        if (g_vecEmitter[0] > 0)
        {
            int iExpect = g_vecEmitter[0] - 1;
            
            int iPrevValue = g_vecEmitter[0];
            
            int iOriginValue = 0;
            
            InterlockedCompareExchange(g_vecEmitter[0], iPrevValue, iPrevValue - 1, iOriginValue);
            
            if (iOriginValue != 0)
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
                g_vecParticleInfo[iDispatchThreadID.x].speed = g_vParticleVelocity;
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
    
    VSOut _point[4] = (VSOut[4]) 0;
    
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            _point[i + j * 2].pos = mul(float4(g_vecParticle[p[0].instID].pos, 1.f), g_matView);
            
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
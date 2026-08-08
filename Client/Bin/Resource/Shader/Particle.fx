#include "shared.hlsl"

// 5x5 가우시안 (블러 PS에서 사용 — 파티클 RNG와는 무관).
static const float g_pGaussianFilter[5][5] =
{
    1 / 273.f, 4 / 273.f, 7 / 273.f, 4 / 273.f, 1 / 273.f,
  4 / 273.f, 16 / 273.f, 26 / 273.f, 16 / 273.f, 4 / 273.f,
  7 / 273.f, 26 / 273.f, 41 / 273.f, 26 / 273.f, 7 / 273.f,
  4 / 273.f, 16 / 273.f, 26 / 273.f, 16 / 273.f, 4 / 273.f,
  1 / 273.f, 4 / 273.f, 7 / 273.f, 4 / 273.f, 1 / 273.f
};

// 정수 해시 PRNG. 기존 노이즈텍스처+cos/sin+가우시안블러 방식은 세 가지 편향이
// 있었다: (1) 가우시안 블러가 분산을 줄여 결과가 노이즈 평균(~0.5)으로 쏠림 →
// 파티클이 스폰 박스 "중앙"으로 뭉침, (2) cos/sin UV가 arcsine 분포라 특정 텍셀
// 과샘플(가장자리 편향), (3) seed*=threadID라 스레드0은 위치 고정·인접 스레드는 상관.
// 균일 [0,1)·스레드/시간 비상관 해시로 교체해 박스 전체를 고르게 채운다.
uint HashU(uint s)
{
    s = s * 747796405u + 2891336453u;
    uint w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
    return (w >> 22u) ^ w;
}

float Rand01(uint s)
{
    return HashU(s) * (1.0 / 4294967296.0); // [0,1)
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
                // 파티클별·프레임별 비상관 시드. 같은 프레임의 서로 다른 파티클은
                // threadID로, 프레임 간에는 accTime 비트(asuint)로 분리된다.
                uint uSeed = HashU(iDispatchThreadID.x * 2654435761u + asuint(g_fGlobalAccTime));

                float3 vRandom = float3
                (
                    Rand01(uSeed + 0x9E3779B9u),
                    Rand01(uSeed + 0x85EBCA77u),
                    Rand01(uSeed + 0xC2B2AE3Du)
                );
                
                g_vecParticleInfo[iDispatchThreadID.x].alive = true;

                g_vecParticleInfo[iDispatchThreadID.x].age = 0.f;
                g_vecParticleInfo[iDispatchThreadID.x].maxage = g_fParticleMaxLifeTime * (1.f + Rand01(uSeed + 0x165667B1u) * 0.2f);
                g_vecParticleInfo[iDispatchThreadID.x].pos = g_vParticleMinimumPosition * (1.f - vRandom) + g_vParticleMaximumPosition * vRandom + g_matWorld[3].xyz;
                g_vecParticleInfo[iDispatchThreadID.x].size = g_vParticleStartSize;
                
                // 주의: vVelocity~vMaxVelocity는 사실상 "방향 분포 박스"로만 동작한다.
                // 아래 normalize가 크기를 버리고 단위 벡터만 남기므로(파티클은 1 unit/sec로
                // 스폰된 뒤 accel로 가속됨), velocity 값의 절대 크기가 아니라 부호·비율
                // (=방향 분포)만 의미가 있다. 물리적 속도 크기를 쓰려면 normalize를 빼고
                // 모든 이펙트의 velocity를 재튜닝해야 한다.
                float3 vSpeed = g_vParticleVelocity * (1.f - vRandom) + g_vParticleMaxVelocity * vRandom;

                // normalize(0)=0/0=NaN 방지. NaN speed는 pos를 오염시켜 빌보드가 화면을
                // 뒤덮는다. 정확히 0뿐 아니라 근사 0도 잡도록 제곱길이로 임계 비교(sqrt 불요).
                if (dot(vSpeed, vSpeed) < 1e-12)
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
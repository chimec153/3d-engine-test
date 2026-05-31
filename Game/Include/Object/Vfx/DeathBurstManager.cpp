#include "DeathBurstManager.h"
#include "Bindable/Transform.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/BlendState.h"
#include "Bindable/DepthStencilState.h"
#include "Bindable/VertexBuffer.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Types.h"
#include <cmath>
#include <string>

namespace Client { DeathBurstManager* DeathBurstManager::m_pInst = nullptr; }

namespace Client
{
    // Named namespace (Game is a unity/jumbo build — avoid anon-namespace
    // collisions across the merged TU).
    namespace death_burst_detail
    {
        constexpr int   kPuffPool   = 160;
        constexpr int   kSparkPool  = 320;
        constexpr int   kRingPool   = 48;
        constexpr int   kPuffPer    = 6;     // puffs per death
        constexpr int   kSparkPer   = 12;    // sparkles per death
        constexpr int   kRampTexels = 16;    // 1xN ramp LUT width
        constexpr float kSparkGravity = -9.f;  // sparkle fall
        constexpr float kSparkHDR   = 2.6f;  // additive tint > 1 -> bloom

        inline unsigned int PackRGBA(float r, float g, float b, float a)
        {
            auto cl = [](float x) { int v = (int)(x * 255.f + 0.5f); return v < 0 ? 0u : (v > 255 ? 255u : (unsigned)v); };
            return cl(r) | (cl(g) << 8) | (cl(b) << 16) | (cl(a) << 24);  // R,G,B,A bytes
        }
    }

    float DeathBurstManager::Rand()
    {
        m_uSeed = m_uSeed * 1664525u + 1013904223u;
        return static_cast<float>(m_uSeed >> 8) * (1.f / 16777216.f);
    }

    std::shared_ptr<Engine::Texture> DeathBurstManager::MakeRamp(
        const std::string& strTag, int iKind, const Engine::Vector3& vBase)
    {
        using namespace death_burst_detail;
        if (auto p = Engine::StaticFindBindable<Engine::Texture>(strTag)) return p;
        auto pTex = Engine::StaticCreateBindable<Engine::Texture>(strTag);
        if (!pTex) return nullptr;

        // Stepped bands (point-sampled in the PS) — flat toon colour over life.
        // Each texel is one band value; alpha ramps to 0 at the tail (the fade).
        std::vector<unsigned int> buf(kRampTexels);
        for (int i = 0; i < kRampTexels; ++i)
        {
            const float u = (i + 0.5f) / kRampTexels;
            float r = 1.f, g = 1.f, b = 1.f, a = 0.f;
            if (iKind == 0)            // puff — white-hot -> enemy colour -> dark -> gone
            {
                if      (u < 0.12f) { r = g = b = 1.f;                                  a = 1.0f; }
                else if (u < 0.32f) { r = vBase.x*.5f+.5f; g = vBase.y*.5f+.5f; b = vBase.z*.5f+.5f; a = 1.0f; }
                else if (u < 0.58f) { r = vBase.x;        g = vBase.y;        b = vBase.z;        a = 1.0f; }
                else if (u < 0.78f) { r = vBase.x*.6f;    g = vBase.y*.6f;    b = vBase.z*.6f;    a = 0.85f; }
                else if (u < 0.90f) { r = vBase.x*.35f;   g = vBase.y*.35f;   b = vBase.z*.35f;   a = 0.5f; }
            }
            else if (iKind == 1)       // sparkle — white -> gold -> gone
            {
                if      (u < 0.15f) { r = 1.f;  g = 1.f;   b = 1.f;   a = 1.0f; }
                else if (u < 0.42f) { r = 1.f;  g = 0.95f; b = 0.65f; a = 1.0f; }
                else if (u < 0.68f) { r = 1.f;  g = 0.80f; b = 0.30f; a = 0.85f; }
                else if (u < 0.88f) { r = 1.f;  g = 0.60f; b = 0.15f; a = 0.45f; }
            }
            else                       // smoke — light grey -> grey -> gone
            {
                if      (u < 0.10f) { r = 0.85f; g = 0.86f; b = 0.92f; a = 0.70f; }
                else if (u < 0.45f) { r = 0.62f; g = 0.62f; b = 0.68f; a = 0.60f; }
                else if (u < 0.78f) { r = 0.45f; g = 0.45f; b = 0.50f; a = 0.38f; }
            }
            buf[i] = death_burst_detail::PackRGBA(r, g, b, a);
        }

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = buf.data();
        init.SysMemPitch = kRampTexels * 4;
        pTex->CreateTexture(kRampTexels, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
        pTex->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        return pTex;
    }

    std::shared_ptr<Engine::Texture> DeathBurstManager::GetPuffRamp(const Engine::Vector3& vColor)
    {
        // Quantise to 4 bits/channel so similar enemy colours share a ramp
        // texture (bounds the cache).
        auto q = [](float x) { int v = (int)(x * 15.f + 0.5f); return v < 0 ? 0u : (v > 15 ? 15u : (unsigned)v); };
        const unsigned int key = (q(vColor.x) << 8) | (q(vColor.y) << 4) | q(vColor.z);
        auto it = m_puffRamps.find(key);
        if (it != m_puffRamps.end()) return it->second;
        auto pRamp = MakeRamp("death_puff_ramp_" + std::to_string(key), 0, vColor);
        m_puffRamps[key] = pRamp;
        return pRamp;
    }

    bool DeathBurstManager::Init()
    {
        using namespace death_burst_detail;
        m_puffs.assign(kPuffPool, Puff{});
        m_sparks.assign(kSparkPool, Spark{});
        m_rings.assign(kRingPool, Ring{});
        m_iNextPuff = m_iNextSpark = m_iNextRing = 0;

        if (!m_bInit)
        {
            m_pVS           = Engine::StaticFindBindable<Engine::VertexShader>("BeamVS");
            m_pPuffPS       = Engine::StaticFindBindable<Engine::PixelShader>("DeathPuffPS");
            m_pStarPS       = Engine::StaticFindBindable<Engine::PixelShader>("DeathStarPS");
            m_pDiamondPS    = Engine::StaticFindBindable<Engine::PixelShader>("DeathDiamondPS");
            m_pRingPS       = Engine::StaticFindBindable<Engine::PixelShader>("DeathRingPS");
            m_pIL           = Engine::StaticFindBindable<Engine::InputLayout>("BeamVtx");
            m_pTopo         = Engine::StaticFindBindable<Engine::Topology>("TriangleList");
            m_pNoCull       = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);
            m_pAlphaBlend   = Engine::StaticFindBindable<Engine::BlendState>("AlphaBlend");
            m_pAddBlend     = Engine::StaticFindBindable<Engine::BlendState>("AccBlend");
            m_pNoDepthWrite = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepthWrite");

            m_pSparkRamp = MakeRamp("death_spark_ramp", 1, Engine::Vector3(1.f, 1.f, 1.f));
            m_pSmokeRamp = MakeRamp("death_smoke_ramp", 2, Engine::Vector3(1.f, 1.f, 1.f));

            m_pVP = std::make_shared<Engine::Transform>();
            m_pVP->SetCameraType(Engine::CAMERA_TYPE::NORMAL);

            Engine::BindableRegistry::Register([]() { DeathBurstManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pVS && m_pPuffPS && m_pStarPS && m_pDiamondPS && m_pRingPS && m_pIL && m_pTopo;
    }

    void DeathBurstManager::SpawnBurst(const Engine::Vector3& vPos, float fScale,
                                       const Engine::Vector3& vColor)
    {
        using namespace death_burst_detail;
        if (m_puffs.empty() || m_sparks.empty() || m_rings.empty()) return;

        auto pRamp = GetPuffRamp(vColor);

        for (int k = 0; k < kPuffPer; ++k)
        {
            Puff& p = m_puffs[m_iNextPuff];
            m_iNextPuff = (m_iNextPuff + 1) % static_cast<int>(m_puffs.size());
            p.vPos  = vPos + Engine::Vector3(Rand(-1.f, 1.f), Rand(0.f, 0.6f), Rand(-1.f, 1.f)) * (fScale * 0.3f);
            p.vVel  = Engine::Vector3(Rand(-1.f, 1.f), Rand(0.4f, 1.2f), Rand(-1.f, 1.f)) * (fScale * 0.7f);
            p.fSize0 = fScale * Rand(0.30f, 0.50f);
            p.fSize1 = fScale * Rand(0.90f, 1.25f);
            p.fLife  = Rand(0.40f, 0.55f);
            p.fAge   = 0.f;
            p.pRamp  = pRamp;
            p.bActive = true;
        }

        for (int k = 0; k < kSparkPer; ++k)
        {
            Spark& s = m_sparks[m_iNextSpark];
            m_iNextSpark = (m_iNextSpark + 1) % static_cast<int>(m_sparks.size());
            const float ang = Rand(0.f, 6.2831853f);
            Engine::Vector3 dir(cosf(ang) * Rand(0.4f, 1.f), Rand(0.3f, 1.f), sinf(ang) * Rand(0.4f, 1.f));
            dir.Normalize();
            s.vPos  = vPos + Engine::Vector3(Rand(-1.f, 1.f), Rand(0.f, 1.f), Rand(-1.f, 1.f)) * (fScale * 0.15f);
            s.vVel  = dir * (fScale * Rand(3.f, 6.f));
            s.fSize = fScale * Rand(0.22f, 0.35f);
            s.iShape = (Rand() < 0.5f) ? 0 : 1;
            s.fLife = Rand(0.40f, 0.60f);
            s.fAge  = 0.f;
            s.bActive = true;
        }

        {
            Ring& r = m_rings[m_iNextRing];
            m_iNextRing = (m_iNextRing + 1) % static_cast<int>(m_rings.size());
            r.vPos   = vPos;
            r.fSize0 = fScale * 0.50f;
            r.fSize1 = fScale * 2.40f;
            r.fLife  = 0.40f;
            r.fAge   = 0.f;
            r.bActive = true;
        }
    }

    void DeathBurstManager::Update(float fDeltaTime)
    {
        using namespace death_burst_detail;
        for (Puff& p : m_puffs)
        {
            if (!p.bActive) continue;
            p.fAge += fDeltaTime;
            if (p.fAge >= p.fLife) { p.bActive = false; p.pRamp = nullptr; continue; }
            p.vPos += p.vVel * fDeltaTime;
        }
        for (Spark& s : m_sparks)
        {
            if (!s.bActive) continue;
            s.fAge += fDeltaTime;
            if (s.fAge >= s.fLife) { s.bActive = false; continue; }
            s.vVel.y += kSparkGravity * fDeltaTime;
            s.vPos += s.vVel * fDeltaTime;
        }
        for (Ring& r : m_rings)
        {
            if (!r.bActive) continue;
            r.fAge += fDeltaTime;
            if (r.fAge >= r.fLife) r.bActive = false;
        }
    }

    void DeathBurstManager::BuildQuad(std::vector<DeathVertex>& out,
        const Engine::Vector3& vCentre, float fHalf,
        const Engine::Vector3& vRight, const Engine::Vector3& vUp,
        float tintR, float tintG, float tintB, float fLifeT)
    {
        const Engine::Vector3 r = vRight * fHalf;
        const Engine::Vector3 u = vUp * fHalf;
        const Engine::Vector3 c0 = vCentre - r - u;   // uv (0,0)
        const Engine::Vector3 c1 = vCentre + r - u;   // uv (1,0)
        const Engine::Vector3 c2 = vCentre + r + u;   // uv (1,1)
        const Engine::Vector3 c3 = vCentre - r + u;   // uv (0,1)
        auto push = [&](const Engine::Vector3& p, float uu, float vv)
        {
            out.push_back({ p.x, p.y, p.z, uu, vv, tintR, tintG, tintB, fLifeT });
        };
        push(c0, 0.f, 0.f); push(c1, 1.f, 0.f); push(c2, 1.f, 1.f);
        push(c0, 0.f, 0.f); push(c2, 1.f, 1.f); push(c3, 0.f, 1.f);
    }

    void DeathBurstManager::Render()
    {
        using namespace death_burst_detail;
        if (!m_bInit || !m_pVS || !m_pPuffPS || !m_pIL || !m_pTopo || !m_pVP) return;

        // Screen-facing billboard basis from the camera.
        Engine::Vector3 vRight(1.f, 0.f, 0.f), vUp(0.f, 1.f, 0.f);
        if (auto pCam = Engine::Graphics::GetInst()->GetCamera())
            if (auto pTr = pCam->GetTransform())
            {
                vRight = pTr->GetAxis(Engine::AXIS_TYPE::X);
                vUp    = pTr->GetAxis(Engine::AXIS_TYPE::Y);
            }

        // Build vertex batches. Puffs group by ramp texture (per enemy type),
        // so each group is one draw with its ramp bound.
        std::vector<std::pair<std::shared_ptr<Engine::Texture>, std::vector<DeathVertex>>> puffGroups;
        m_vRing.clear();
        m_vStar.clear();
        m_vDiamond.clear();

        for (const Puff& p : m_puffs)
        {
            if (!p.bActive || !p.pRamp) continue;
            const float t = p.fAge / p.fLife;
            const float f = 1.f - (1.f - t) * (1.f - t);          // ease-out expand
            const float half = (p.fSize0 + (p.fSize1 - p.fSize0) * f) * 0.5f;
            std::vector<DeathVertex>* pOut = nullptr;
            for (auto& g : puffGroups) if (g.first == p.pRamp) { pOut = &g.second; break; }
            if (!pOut) { puffGroups.push_back({ p.pRamp, {} }); pOut = &puffGroups.back().second; }
            BuildQuad(*pOut, p.vPos, half, vRight, vUp, 1.f, 1.f, 1.f, t);
        }
        for (const Ring& r : m_rings)
        {
            if (!r.bActive) continue;
            const float t = r.fAge / r.fLife;
            const float f = 1.f - (1.f - t) * (1.f - t);
            const float half = (r.fSize0 + (r.fSize1 - r.fSize0) * f) * 0.5f;
            BuildQuad(m_vRing, r.vPos, half, vRight, vUp, 1.f, 1.f, 1.f, t);
        }
        for (const Spark& s : m_sparks)
        {
            if (!s.bActive) continue;
            const float t = s.fAge / s.fLife;
            const float twinkle = 0.7f + 0.3f * sinf(s.fAge * 25.f);
            const float half = s.fSize * twinkle * 0.5f;
            std::vector<DeathVertex>& out = (s.iShape == 0) ? m_vStar : m_vDiamond;
            BuildQuad(out, s.vPos, half, vRight, vUp, kSparkHDR, kSparkHDR, kSparkHDR, t);
        }

        bool bAny = !m_vRing.empty() || !m_vStar.empty() || !m_vDiamond.empty();
        for (auto& g : puffGroups) if (!g.second.empty()) { bAny = true; break; }
        if (!bAny) return;

        auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

        m_pNoDepthWrite->Bind();
        m_pNoCull->Bind();
        m_pVP->SetScale(1.f, 1.f, 1.f);
        m_pVP->SetPosition(0.f, 0.f, 0.f);
        m_pVP->PostUpdate(0.f);
        m_pVP->Bind();
        m_pVS->Bind();
        m_pIL->Bind();
        m_pTopo->Bind();

        auto drawBatch = [&](std::vector<DeathVertex>& verts)
        {
            if (verts.empty()) return;
            auto pVB = std::make_shared<Engine::VertexBuffer>(verts);
            pVB->Bind();
            pDC->Draw(static_cast<UINT>(verts.size()), 0);
        };

        // ---- Alpha-blended: puffs (per-type ramp) + smoke ring ----
        m_pAlphaBlend->Bind();
        m_pPuffPS->Bind();
        for (auto& g : puffGroups)
        {
            if (g.second.empty() || !g.first) continue;
            g.first->Bind();              // ramp -> t0
            drawBatch(g.second);
        }
        if (!m_vRing.empty() && m_pRingPS && m_pSmokeRamp)
        {
            m_pRingPS->Bind();
            m_pSmokeRamp->Bind();
            drawBatch(m_vRing);
        }

        // ---- Additive: star + diamond sparkles (shared spark ramp) ----
        if ((!m_vStar.empty() || !m_vDiamond.empty()) && m_pSparkRamp)
        {
            m_pAddBlend->Bind();
            m_pSparkRamp->Bind();
            if (!m_vStar.empty())    { m_pStarPS->Bind();    drawBatch(m_vStar); }
            if (!m_vDiamond.empty()) { m_pDiamondPS->Bind(); drawBatch(m_vDiamond); }
            m_pAddBlend->PostBind();   // restore AlphaBlend
        }

        m_pAlphaBlend->PostBind();     // restore the pass default
        m_pNoCull->PostBind();
        m_pNoDepthWrite->PostBind();

        ID3D11ShaderResourceView* pNull[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNull);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }

    void DeathBurstManager::Clear()
    {
        for (Puff& p : m_puffs)  { p.bActive = false; p.pRamp = nullptr; }
        for (Spark& s : m_sparks)  s.bActive = false;
        for (Ring& r : m_rings)    r.bActive = false;
        m_iNextPuff = m_iNextSpark = m_iNextRing = 0;
    }
}

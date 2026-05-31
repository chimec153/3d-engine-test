#include "TrailRenderManager.h"
#include "Bindable/Transform.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/BlendState.h"
#include "Bindable/DepthStencilState.h"
#include "Bindable/VertexBuffer.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Types.h"
#include <cmath>

namespace Client { TrailRenderManager* TrailRenderManager::m_pInst = nullptr; }

namespace Client
{
    // Named (not anonymous) namespace: the Game project is a unity/jumbo build,
    // so an anonymous-namespace helper here could collide with same-named
    // helpers in another file merged into the same translation unit.
    namespace trail_render_detail
    {
        // Preset table, indexed by (int)TrailStyle. Tracer reproduces the
        // original look. Tune trail looks here.
        //                       maxPts minDist wMul  tailFr headInt glowSc glowInt
        const TrailPreset kPresets[] =
        {
            {  0, 0.00f, 0.0f, 0.00f, 0.0f, 0.0f, 0.0f },   // None  — no trail
            { 20, 0.18f, 1.0f, 0.12f, 4.0f, 2.2f, 5.5f },   // Tracer (default)
            { 28, 0.16f, 1.8f, 0.25f, 6.5f, 3.2f, 8.0f },   // Plasma — thick/bright/long
            {  8, 0.12f, 0.7f, 0.05f, 3.5f, 1.6f, 4.0f },   // Spark  — short/thin/snappy
            { 36, 0.20f, 1.2f, 0.35f, 5.0f, 2.6f, 6.5f },   // Comet  — long fading tail
        };
        constexpr int kPresetCount = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));
    }

    const TrailPreset& TrailRenderManager::GetPreset(TrailStyle eStyle)
    {
        int i = static_cast<int>(eStyle);
        if (i < 0 || i >= trail_render_detail::kPresetCount)
            i = static_cast<int>(TrailStyle::Tracer);
        return trail_render_detail::kPresets[i];
    }

    bool TrailRenderManager::Init()
    {
        m_subs.clear();

        if (!m_bInit)
        {
            m_pVS           = Engine::StaticFindBindable<Engine::VertexShader>("BeamVS");
            m_pPS           = Engine::StaticFindBindable<Engine::PixelShader>("BeamPS");
            m_pGlowPS       = Engine::StaticFindBindable<Engine::PixelShader>("BeamGlowPS");
            m_pIL           = Engine::StaticFindBindable<Engine::InputLayout>("BeamVtx");
            m_pTopo         = Engine::StaticFindBindable<Engine::Topology>("TriangleList");
            m_pNoCull       = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);
            m_pAddBlend     = Engine::StaticFindBindable<Engine::BlendState>("AccBlend");
            m_pNoDepthWrite = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepthWrite");

            m_pVP = std::make_shared<Engine::Transform>();
            m_pVP->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // identity world -> g_matTransform = VP

            Engine::BindableRegistry::Register([]() { TrailRenderManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pVS && m_pPS && m_pIL && m_pTopo;
    }

    void TrailRenderManager::Submit(const std::deque<Engine::Vector3>& path,
                                    const Engine::Vector3& vColor, float fBulletRadius,
                                    TrailStyle eStyle)
    {
        if (path.size() < 2 || fBulletRadius <= 0.f) return;
        if (GetPreset(eStyle).iMaxPoints <= 0) return;   // None — nothing to draw
        Trail t;
        t.path.assign(path.begin(), path.end());
        t.vColor = vColor;
        t.fBulletRadius = fBulletRadius;
        t.eStyle = eStyle;
        m_subs.push_back(std::move(t));
    }

    void TrailRenderManager::Render()
    {
        using namespace trail_render_detail;

        if (m_bInit && !m_subs.empty() &&
            m_pVS && m_pPS && m_pGlowPS && m_pIL && m_pTopo && m_pVP &&
            m_pNoCull && m_pAddBlend && m_pNoDepthWrite)
        {
            // Camera basis: eye for the ribbon billboard width axis, right/up
            // for the screen-facing head glow.
            Engine::Vector3 vEye(0.f, 0.f, 0.f);
            Engine::Vector3 vRight(1.f, 0.f, 0.f);
            Engine::Vector3 vUp(0.f, 1.f, 0.f);
            if (auto pCam = Engine::Graphics::GetInst()->GetCamera())
                if (auto pCamTr = pCam->GetTransform())
                {
                    vEye   = pCamTr->GetPosition();
                    vRight = pCamTr->GetAxis(Engine::AXIS_TYPE::X);
                    vUp    = pCamTr->GetAxis(Engine::AXIS_TYPE::Y);
                }

            m_ribbon.clear();
            m_glows.clear();

            for (const Trail& tr : m_subs)
            {
                const int n = static_cast<int>(tr.path.size());
                if (n < 2) continue;
                const float fInvLast = 1.f / static_cast<float>(n - 1);

                const TrailPreset& ps = GetPreset(tr.eStyle);
                const float fHeadHalf = tr.fBulletRadius * ps.fHeadWidthMul;

                // Accumulated distance from the head, for an even-flowing UV.u.
                float fCum = 0.f;

                for (int i = 0; i + 1 < n; ++i)
                {
                    const Engine::Vector3& p0 = tr.path[i];
                    const Engine::Vector3& p1 = tr.path[i + 1];

                    Engine::Vector3 vSeg = p1 - p0;
                    const float fSegLen = vSeg.Length();
                    if (fSegLen < 1e-5f) continue;
                    const Engine::Vector3 vDir = vSeg * (1.f / fSegLen);

                    // Camera-facing width axis for this segment.
                    Engine::Vector3 vMid  = (p0 + p1) * 0.5f;
                    Engine::Vector3 vSide = vDir.Cross(vEye - vMid);
                    if (vSide.LengthSq() < 1e-6f) vSide = vDir.Cross(Engine::Vector3(0.f, 1.f, 0.f));
                    if (vSide.LengthSq() < 1e-6f) vSide = vDir.Cross(Engine::Vector3(1.f, 0.f, 0.f));
                    vSide.Normalize();

                    // Taper width + brightness head(0) -> tail(1).
                    const float t0 = static_cast<float>(i)     * fInvLast;
                    const float t1 = static_cast<float>(i + 1) * fInvLast;
                    const float h0 = fHeadHalf * (1.f - t0 * (1.f - ps.fTailWidthFrac));
                    const float h1 = fHeadHalf * (1.f - t1 * (1.f - ps.fTailWidthFrac));
                    const float in0 = ps.fHeadIntensity * (1.f - t0);
                    const float in1 = ps.fHeadIntensity * (1.f - t1);
                    const float u0 = fCum;
                    const float u1 = fCum + fSegLen;
                    fCum = u1;

                    const Engine::Vector3 o0 = vSide * h0;
                    const Engine::Vector3 o1 = vSide * h1;
                    const Engine::Vector3 a = p0 - o0;   // uv (u0,0)
                    const Engine::Vector3 b = p1 - o1;   // uv (u1,0)
                    const Engine::Vector3 c = p1 + o1;   // uv (u1,1)
                    const Engine::Vector3 d = p0 + o0;   // uv (u0,1)

                    auto push = [&](const Engine::Vector3& p, float u, float v, float in)
                    {
                        m_ribbon.push_back({ p.x, p.y, p.z, u, v,
                                             tr.vColor.x * in, tr.vColor.y * in, tr.vColor.z * in, 1.f });
                    };
                    push(a, u0, 0.f, in0); push(b, u1, 0.f, in1); push(c, u1, 1.f, in1);
                    push(a, u0, 0.f, in0); push(c, u1, 1.f, in1); push(d, u0, 1.f, in0);
                }

                // Head glow: a screen-facing additive quad at the tip.
                {
                    const Engine::Vector3& head = tr.path[0];
                    const float s = fHeadHalf * ps.fGlowScale;
                    const Engine::Vector3 r = vRight * s;
                    const Engine::Vector3 u = vUp * s;
                    const float gr = tr.vColor.x * ps.fGlowIntensity;
                    const float gg = tr.vColor.y * ps.fGlowIntensity;
                    const float gb = tr.vColor.z * ps.fGlowIntensity;
                    const Engine::Vector3 g0 = head - r - u;   // uv (0,0)
                    const Engine::Vector3 g1 = head + r - u;   // uv (1,0)
                    const Engine::Vector3 g2 = head + r + u;   // uv (1,1)
                    const Engine::Vector3 g3 = head - r + u;   // uv (0,1)
                    auto pushG = [&](const Engine::Vector3& p, float uu, float vv)
                    {
                        m_glows.push_back({ p.x, p.y, p.z, uu, vv, gr, gg, gb, 1.f });
                    };
                    pushG(g0, 0.f, 0.f); pushG(g1, 1.f, 0.f); pushG(g2, 1.f, 1.f);
                    pushG(g0, 0.f, 0.f); pushG(g2, 1.f, 1.f); pushG(g3, 0.f, 1.f);
                }
            }

            if (!m_ribbon.empty() || !m_glows.empty())
            {
                auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

                m_pAddBlend->Bind();        // additive ONE/ONE
                m_pNoDepthWrite->Bind();    // depth test on, write off
                m_pNoCull->Bind();

                m_pVP->SetScale(1.f, 1.f, 1.f);
                m_pVP->SetPosition(0.f, 0.f, 0.f);
                m_pVP->PostUpdate(0.f);
                m_pVP->Bind();              // g_matTransform = VP

                m_pVS->Bind();
                m_pIL->Bind();
                m_pTopo->Bind();

                if (!m_ribbon.empty())
                {
                    m_pPS->Bind();
                    auto pVB = std::make_shared<Engine::VertexBuffer>(m_ribbon);
                    pVB->Bind();
                    pDC->Draw(static_cast<UINT>(m_ribbon.size()), 0);
                }
                if (!m_glows.empty())
                {
                    m_pGlowPS->Bind();
                    auto pVB = std::make_shared<Engine::VertexBuffer>(m_glows);
                    pVB->Bind();
                    pDC->Draw(static_cast<UINT>(m_glows.size()), 0);
                }

                m_pNoCull->PostBind();
                m_pNoDepthWrite->PostBind();
                m_pAddBlend->PostBind();

                pDC->VSSetShader(nullptr, nullptr, 0);
                pDC->PSSetShader(nullptr, nullptr, 0);
                Engine::Graphics::GetInst()->ResetBindCache();
            }
        }

        m_subs.clear();
    }
}

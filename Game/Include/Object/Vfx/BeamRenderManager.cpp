#include "BeamRenderManager.h"
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

namespace Client { BeamRenderManager* BeamRenderManager::m_pInst = nullptr; }

namespace Client
{
    // Per-layer look. The classic laser stack: a wide, saturated, gently-HDR
    // outer glow + a mid body + a thin, near-white, strongly-HDR core. Reusing
    // BeamVS/BeamPS, only the width and colour change. whiten lerps the beam
    // colour toward white; intensity scales it past 1.0 so bloom haloes it.
    //
    // Named (not anonymous) namespace: the Game project is a unity/jumbo build,
    // so an anonymous-namespace helper here could collide with same-named
    // helpers in another file merged into the same TU.
    namespace beam_render_detail
    {
        struct BeamLayer { float fWidthMul; float fIntensity; float fWhiten; };
        const BeamLayer kLayers[] =
        {
            { 2.6f, 1.6f, 0.00f },   // outer glow — wide, saturated colour
            { 1.1f, 3.5f, 0.35f },   // body
            { 0.40f, 8.0f, 0.85f },  // hot core — thin, near-white, blooms
        };
        constexpr int kLayerCount = static_cast<int>(sizeof(kLayers) / sizeof(kLayers[0]));
    }

    bool BeamRenderManager::Init()
    {
        m_subs.clear();

        if (!m_bInit)
        {
            m_pVS           = Engine::StaticFindBindable<Engine::VertexShader>("BeamVS");
            m_pPS           = Engine::StaticFindBindable<Engine::PixelShader>("BeamPS");
            m_pIL           = Engine::StaticFindBindable<Engine::InputLayout>("BeamVtx");
            m_pTopo         = Engine::StaticFindBindable<Engine::Topology>("TriangleList");
            m_pNoCull       = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);
            m_pAddBlend     = Engine::StaticFindBindable<Engine::BlendState>("AccBlend");
            m_pNoDepthWrite = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepthWrite");

            m_pVP = std::make_shared<Engine::Transform>();
            m_pVP->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP; identity world -> g_matTransform = VP

            Engine::BindableRegistry::Register([]() { BeamRenderManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pVS && m_pPS && m_pIL && m_pTopo;
    }

    void BeamRenderManager::Submit(const Engine::Vector3& vStart, const Engine::Vector3& vEnd,
                                   float fHalfWidth, const Engine::Vector3& vColor)
    {
        if (fHalfWidth <= 0.f) return;
        m_subs.push_back({ vStart, vEnd, fHalfWidth, vColor });
    }

    void BeamRenderManager::Render()
    {
        using namespace beam_render_detail;

        if (m_bInit && !m_subs.empty() &&
            m_pVS && m_pPS && m_pIL && m_pTopo && m_pVP &&
            m_pNoCull && m_pAddBlend && m_pNoDepthWrite)
        {
            // Camera world position drives the billboard width axis.
            Engine::Vector3 vEye(0.f, 0.f, 0.f);
            if (auto pCam = Engine::Graphics::GetInst()->GetCamera())
                if (auto pCamTr = pCam->GetTransform())
                    vEye = pCamTr->GetPosition();

            // Build all quads (every beam x every layer) into one batch.
            m_verts.clear();
            m_verts.reserve(m_subs.size() * kLayerCount * 6);

            for (const Seg& s : m_subs)
            {
                Engine::Vector3 vDir = s.vEnd - s.vStart;
                const float fLen = vDir.Length();
                if (fLen < 1e-4f) continue;
                vDir = vDir * (1.f / fLen);

                // Cylindrical billboard: width axis perpendicular to both the
                // beam and the eye direction, so the ribbon faces the camera.
                Engine::Vector3 vMid  = (s.vStart + s.vEnd) * 0.5f;
                Engine::Vector3 vSide = vDir.Cross(vEye - vMid);
                if (vSide.LengthSq() < 1e-6f)             // looking down the beam
                    vSide = vDir.Cross(Engine::Vector3(0.f, 1.f, 0.f));
                if (vSide.LengthSq() < 1e-6f)
                    vSide = vDir.Cross(Engine::Vector3(1.f, 0.f, 0.f));
                vSide.Normalize();

                for (int L = 0; L < kLayerCount; ++L)
                {
                    const BeamLayer& lay = kLayers[L];
                    const float fHalf = s.fHalfWidth * lay.fWidthMul;
                    // Layer colour = beam colour whitened toward 1, then scaled
                    // past 1.0 for HDR. r/g/b can exceed 1 (feeds bloom).
                    const float cr = (s.vColor.x + (1.f - s.vColor.x) * lay.fWhiten) * lay.fIntensity;
                    const float cg = (s.vColor.y + (1.f - s.vColor.y) * lay.fWhiten) * lay.fIntensity;
                    const float cb = (s.vColor.z + (1.f - s.vColor.z) * lay.fWhiten) * lay.fIntensity;

                    const Engine::Vector3 o = vSide * fHalf;
                    const Engine::Vector3 a = s.vStart - o;   // uv (0,0)
                    const Engine::Vector3 b = s.vEnd   - o;   // uv (1,0)
                    const Engine::Vector3 c = s.vEnd   + o;   // uv (1,1)
                    const Engine::Vector3 d = s.vStart + o;   // uv (0,1)

                    auto push = [&](const Engine::Vector3& p, float u, float v)
                    {
                        m_verts.push_back({ p.x, p.y, p.z, u, v, cr, cg, cb, 1.f });
                    };
                    // Two triangles: a-b-c, a-c-d.
                    push(a, 0.f, 0.f); push(b, 1.f, 0.f); push(c, 1.f, 1.f);
                    push(a, 0.f, 0.f); push(c, 1.f, 1.f); push(d, 0.f, 1.f);
                }
            }

            if (!m_verts.empty())
            {
                auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

                m_pAddBlend->Bind();        // additive ONE/ONE (restores AlphaBlend on PostBind)
                m_pNoDepthWrite->Bind();    // depth test on, write off
                m_pNoCull->Bind();          // billboard winding flips with view

                // Identity NORMAL transform -> g_matTransform = VP, so the
                // world-space corners project straight to clip space.
                m_pVP->SetScale(1.f, 1.f, 1.f);
                m_pVP->SetPosition(0.f, 0.f, 0.f);
                m_pVP->PostUpdate(0.f);
                m_pVP->Bind();

                m_pVS->Bind();
                m_pPS->Bind();
                m_pIL->Bind();
                m_pTopo->Bind();

                auto pVB = std::make_shared<Engine::VertexBuffer>(m_verts);
                pVB->Bind();
                pDC->Draw(static_cast<UINT>(m_verts.size()), 0);

                // Restore the alpha pass's blend / depth / raster state.
                m_pNoCull->PostBind();
                m_pNoDepthWrite->PostBind();
                m_pAddBlend->PostBind();

                pDC->VSSetShader(nullptr, nullptr, 0);
                pDC->PSSetShader(nullptr, nullptr, 0);
                Engine::Graphics::GetInst()->ResetBindCache();
            }
        }

        // Always drain the queue — Submit re-fills it next frame.
        m_subs.clear();
    }
}

#include "FootstepManager.h"
#include "Bindable/Texture.h"
#include "Bindable/Transform.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Mesh.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/ConstantBuffer.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Types.h"
#include <cmath>
#include <vector>

namespace Client { FootstepManager* FootstepManager::m_pInst = nullptr; }

namespace Client
{
    std::shared_ptr<Engine::Texture> FootstepManager::EnsureFootTexture()
    {
        if (auto p = Engine::StaticFindBindable<Engine::Texture>("footprint_tex"))
            return p;
        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("footprint_tex");
        if (!pNew) return nullptr;

        // Procedural footprint shape: a soft ellipse, longer along the forward
        // (v) axis. RGB is white (PS_UITint ignores it); alpha is the shape.
        constexpr int W = 64, H = 64;
        std::vector<unsigned int> buf(W * H);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const float u = (x + 0.5f) / W - 0.5f;   // -0.5 .. 0.5
                const float v = (y + 0.5f) / H - 0.5f;
                const float du = u / 0.30f;
                const float dv = v / 0.42f;               // taller => elongated forward
                float a = 1.f - (du * du + dv * dv);      // >0 inside the ellipse
                if (a < 0.f) a = 0.f;
                if (a > 1.f) a = 1.f;
                const unsigned int ai = static_cast<unsigned int>(a * 255.f);
                // ABGR memory layout (bytes R,G,B,A) - matches EnsureSolidTexture.
                buf[y * W + x] = (ai << 24) | (255u << 16) | (255u << 8) | 255u;
            }
        }

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = buf.data();
        init.SysMemPitch = W * 4;
        pNew->CreateTexture(W, H, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
        pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        return pNew;
    }

    bool FootstepManager::Init()
    {
        // Pool always resets on (re)init; resources build once.
        m_pool.assign(kPoolSize, Step{});
        m_iNext = 0;

        if (!m_bInit)
        {
            m_pTex  = EnsureFootTexture();
            m_pVS   = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
            m_pPS   = Engine::StaticFindBindable<Engine::PixelShader> ("UIPSTint");
            m_pMesh = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
            m_pTopo = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
            m_pTint = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UITINTBUFFER>>("UITint");
            m_pUI   = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UICBUFFER>>("UI");
            m_pNoCull = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);

            m_pQuad = std::make_shared<Engine::Transform>();
            m_pQuad->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP for UIVS

            // Destroy the singleton at app shutdown (Client/Editor mains call
            // BindableRegistry::DestroyAll). Mirrors DamageTextManager's hook
            // so the new'd instance + its texture/transform are released before
            // the device dies. Registered once (guarded by m_bInit).
            Engine::BindableRegistry::Register([]() { FootstepManager::DestroyInst(); });

            m_bInit = true;
        }
        return m_pTex && m_pVS && m_pPS && m_pMesh;
    }

    void FootstepManager::Spawn(const Engine::Vector3& vPos, float fYaw)
    {
        if (m_pool.empty()) return;
        Step& s = m_pool[m_iNext % static_cast<int>(m_pool.size())];
        m_iNext = (m_iNext + 1) % static_cast<int>(m_pool.size());
        s.vPos    = vPos;
        s.fYaw    = fYaw;
        s.fAge    = 0.f;
        s.bActive = true;
    }

    void FootstepManager::Update(float fDeltaTime)
    {
        for (Step& s : m_pool)
        {
            if (!s.bActive) continue;
            s.fAge += fDeltaTime;
            if (s.fAge >= kLifetime) s.bActive = false;
        }
    }

    void FootstepManager::Render()
    {
        if (!m_bInit || !m_pVS || !m_pPS || !m_pMesh || !m_pQuad || !m_pTex) return;

        bool bAny = false;
        for (const Step& s : m_pool) if (s.bActive) { bAny = true; break; }
        if (!bAny) return;

        auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

        // Full-quad UV on the shared b5 cbuffer (a previous UI draw may have
        // left a sub-region). Bound once for the batch.
        if (m_pUI)
        {
            Engine::UICBUFFER ui{};
            ui.vStartUV = Engine::Vector2(0.f, 0.f);
            ui.vEndUV   = Engine::Vector2(1.f, 1.f);
            m_pUI->UpdateBuffer(ui);
            m_pUI->Bind();
        }

        m_pVS->Bind();
        m_pPS->Bind();
        if (m_pTopo) m_pTopo->Bind();
        // Ground quads are rotated flat, so their winding faces away from the
        // camera at some yaws - disable culling so they always show (the
        // opaque pass leaves CULL_BACK bound, which otherwise hides them).
        if (m_pNoCull) m_pNoCull->Bind();
        // UI VS reads SV_VertexID, not VB inputs - clear the input layout so a
        // leftover IL from the opaque pass doesn't mismatch (see UIRenderer).
        pDC->IASetInputLayout(nullptr);
        Engine::Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;
        m_pTex->Bind();   // slot 0; sampled for alpha by PS_UITint

        for (const Step& s : m_pool)
        {
            if (!s.bActive) continue;

            const float fFade  = 1.f - s.fAge / kLifetime;   // 1 -> 0
            const float fAlpha = fFade * kMaxAlpha;

            // Centre the local (0,0)-(1,1) UI quad on the foot. The quad's
            // world half-diagonal is 0.5*kSize*(u+v) where u,v are the
            // transform's rotated X/Y axes; read them back after a first
            // PostUpdate so the offset is exact regardless of Euler order
            // (an analytic guess at the X/Y rotation order was slightly off).
            m_pQuad->SetScale(kSize, kSize, 1.f);
            m_pQuad->SetRX(-PI / 2.f);     // lay the XY quad onto the XZ ground
            m_pQuad->SetRY(s.fYaw);
            m_pQuad->SetPosition(s.vPos);
            m_pQuad->PostUpdate(0.f);      // compute rotated axes + matrix
            const Engine::Vector3 u = m_pQuad->GetAxis(Engine::AXIS_TYPE::X);
            const Engine::Vector3 v = m_pQuad->GetAxis(Engine::AXIS_TYPE::Y);
            m_pQuad->SetPosition(s.vPos - (u + v) * (0.5f * kSize));
            m_pQuad->PostUpdate(0.f);      // recompose at the centred position
            m_pQuad->Bind();               // g_matTransform = world * VP

            if (m_pTint)
            {
                Engine::UITINTBUFFER tint{};
                tint.vTint = Engine::Vector4(0.12f, 0.10f, 0.08f, fAlpha);  // dark mark
                m_pTint->UpdateBuffer(tint);
                m_pTint->Bind();
            }

            m_pMesh->Draw();
        }

        // Restore a clean slot 0 / VS / PS / cull state for whatever renders next.
        if (m_pNoCull) m_pNoCull->PostBind();
        ID3D11ShaderResourceView* pNull[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNull);
        pDC->VSSetShaderResources(0, 1, pNull);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }

    void FootstepManager::Clear()
    {
        for (Step& s : m_pool) s.bActive = false;
        m_iNext = 0;
    }
}

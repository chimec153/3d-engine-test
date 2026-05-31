#include "HealAuraManager.h"
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

namespace Client { HealAuraManager* HealAuraManager::m_pInst = nullptr; }

namespace Client
{
    std::shared_ptr<Engine::Texture> HealAuraManager::EnsureCircleTexture()
    {
        if (auto p = Engine::StaticFindBindable<Engine::Texture>("heal_circle_tex"))
            return p;
        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("heal_circle_tex");
        if (!pNew) return nullptr;

        // Soft-edged filled disc with a brighter rim, so it reads as a ring
        // that fills. RGB white (PS_UITint ignores it); alpha is the shape.
        constexpr int W = 64, H = 64;
        std::vector<unsigned int> buf(W * H);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const float u = (x + 0.5f) / W - 0.5f;
                const float v = (y + 0.5f) / H - 0.5f;
                const float r = std::sqrt(u * u + v * v) * 2.f;   // 0 centre .. 1 edge
                float a = 0.f;
                if (r <= 1.f)
                {
                    // Faint fill inside + a bright rim near the edge.
                    a = 0.28f;
                    const float rim = 1.f - std::fabs(r - 0.9f) / 0.12f;
                    if (rim > 0.f) a = (std::max)(a, rim);
                }
                if (a > 1.f) a = 1.f;
                const unsigned int ai = static_cast<unsigned int>(a * 255.f);
                // ABGR memory layout (bytes R,G,B,A).
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

    bool HealAuraManager::Init()
    {
        m_subs.clear();

        if (!m_bInit)
        {
            m_pTex  = EnsureCircleTexture();
            m_pVS   = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
            m_pPS   = Engine::StaticFindBindable<Engine::PixelShader> ("UIPSTint");
            m_pMesh = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
            m_pTopo = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
            m_pTint = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UITINTBUFFER>>("UITint");
            m_pUI   = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UICBUFFER>>("UI");
            m_pNoCull = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);

            m_pQuad = std::make_shared<Engine::Transform>();
            m_pQuad->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP for UIVS

            Engine::BindableRegistry::Register([]() { HealAuraManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pTex && m_pVS && m_pPS && m_pMesh;
    }

    void HealAuraManager::Submit(const Engine::Vector3& vCentre, float fRadius, float fAlpha)
    {
        if (fRadius <= 0.01f) return;
        m_subs.push_back({ vCentre, fRadius, fAlpha });
    }

    void HealAuraManager::Render()
    {
        if (!m_bInit || m_subs.empty()) return;
        if (!m_pVS || !m_pPS || !m_pMesh || !m_pQuad || !m_pTex) return;

        auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

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
        if (m_pTopo)   m_pTopo->Bind();
        if (m_pNoCull) m_pNoCull->Bind();   // flat ground quad faces either way
        // UIVS reads SV_VertexID, not VB inputs — clear the input layout so a
        // leftover IL from the opaque pass doesn't mismatch.
        pDC->IASetInputLayout(nullptr);
        Engine::Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;
        m_pTex->Bind();   // slot 0; sampled for alpha by PS_UITint

        for (const Aura& a : m_subs)
        {
            // Diameter-sized quad laid flat on the ground, centred on the tower.
            const float fSize = a.fRadius * 2.f;
            m_pQuad->SetScale(fSize, fSize, 1.f);
            m_pQuad->SetRX(-PI / 2.f);     // lay the XY quad onto the XZ ground
            m_pQuad->SetRY(0.f);
            m_pQuad->SetPosition(a.vCentre);
            m_pQuad->PostUpdate(0.f);      // compute rotated axes + matrix
            const Engine::Vector3 ux = m_pQuad->GetAxis(Engine::AXIS_TYPE::X);
            const Engine::Vector3 uy = m_pQuad->GetAxis(Engine::AXIS_TYPE::Y);
            m_pQuad->SetPosition(a.vCentre - (ux + uy) * (0.5f * fSize));
            m_pQuad->PostUpdate(0.f);      // recompose centred
            m_pQuad->Bind();

            if (m_pTint)
            {
                Engine::UITINTBUFFER tint{};
                tint.vTint = Engine::Vector4(0.2f, 1.0f, 0.4f, a.fAlpha);  // green
                m_pTint->UpdateBuffer(tint);
                m_pTint->Bind();
            }

            m_pMesh->Draw();
        }

        if (m_pNoCull) m_pNoCull->PostBind();
        ID3D11ShaderResourceView* pNull[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNull);
        pDC->VSSetShaderResources(0, 1, pNull);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }
}

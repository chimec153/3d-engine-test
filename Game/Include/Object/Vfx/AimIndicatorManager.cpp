#include "AimIndicatorManager.h"
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

namespace Client { AimIndicatorManager* AimIndicatorManager::m_pInst = nullptr; }

namespace Client
{
    std::shared_ptr<Engine::Texture> AimIndicatorManager::EnsureTriangleTexture()
    {
        if (auto p = Engine::StaticFindBindable<Engine::Texture>("aim_triangle_tex"))
            return p;
        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("aim_triangle_tex");
        if (!pNew) return nullptr;

        // Procedural triangle: tip at large y (the +v / forward axis once the
        // quad is laid flat with RX(-PI/2)), widening to a base at y=0. RGB is
        // white (PS_UITint ignores it); alpha is the triangle shape.
        constexpr int W = 64, H = 64;
        std::vector<unsigned int> buf(W * H);
        for (int y = 0; y < H; ++y)
        {
            const float fy   = (y + 0.5f) / H;        // 0 (base) .. 1 (tip)
            const float half = 0.5f * (1.f - fy);     // wide at base, point at tip
            for (int x = 0; x < W; ++x)
            {
                const float fx = (x + 0.5f) / W - 0.5f;   // -0.5 .. 0.5
                // Signed distance into the triangle; soft-edge over ~1.5px.
                float a = (half - std::abs(fx)) * static_cast<float>(W) / 1.5f;
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

    bool AimIndicatorManager::Init()
    {
        m_bVisible = false;

        if (!m_bInit)
        {
            m_pTex  = EnsureTriangleTexture();
            m_pVS   = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
            m_pPS   = Engine::StaticFindBindable<Engine::PixelShader> ("UIPSTint");
            m_pMesh = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
            m_pTopo = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
            m_pTint = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UITINTBUFFER>>("UITint");
            m_pUI   = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UICBUFFER>>("UI");
            m_pNoCull = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);

            m_pQuad = std::make_shared<Engine::Transform>();
            m_pQuad->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP for UIVS

            Engine::BindableRegistry::Register([]() { AimIndicatorManager::DestroyInst(); });

            m_bInit = true;
        }
        return m_pTex && m_pVS && m_pPS && m_pMesh;
    }

    void AimIndicatorManager::Set(const Engine::Vector3& vPos, float fYaw)
    {
        m_vPos     = vPos;
        m_fYaw     = fYaw;
        m_bVisible = true;
    }

    void AimIndicatorManager::Render()
    {
        if (!m_bVisible || !m_bInit || !m_pVS || !m_pPS || !m_pMesh || !m_pQuad || !m_pTex)
            return;

        auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

        // Full-quad UV on the shared b5 cbuffer (a previous UI draw may have
        // left a sub-region).
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
        if (m_pNoCull) m_pNoCull->Bind();
        // UIVS reads SV_VertexID, not VB inputs - clear the input layout so a
        // leftover IL from the opaque pass doesn't mismatch (see UIRenderer).
        pDC->IASetInputLayout(nullptr);
        Engine::Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;
        m_pTex->Bind();   // slot 0; sampled for alpha by PS_UITint

        // Centre the local (0,0)-(1,1) UI quad on the player feet. The world
        // half-diagonal is 0.5*kSize*(u+v) where u,v are the transform's rotated
        // X/Y axes; read them back after a first PostUpdate so the offset is
        // exact regardless of Euler order (same trick as FootstepManager).
        m_pQuad->SetScale(kSize, kSize, 1.f);
        m_pQuad->SetRX(-PI / 2.f);     // lay the XY quad onto the XZ ground
        m_pQuad->SetRY(m_fYaw);        // tip down the forward axis -> cursor
        m_pQuad->SetPosition(m_vPos);
        m_pQuad->PostUpdate(0.f);
        const Engine::Vector3 u = m_pQuad->GetAxis(Engine::AXIS_TYPE::X);
        const Engine::Vector3 v = m_pQuad->GetAxis(Engine::AXIS_TYPE::Y);
        m_pQuad->SetPosition(m_vPos - (u + v) * (0.5f * kSize));
        m_pQuad->PostUpdate(0.f);
        m_pQuad->Bind();               // g_matTransform = world * VP

        if (m_pTint)
        {
            Engine::UITINTBUFFER tint{};
            tint.vTint = Engine::Vector4(0.30f, 0.90f, 1.00f, 0.55f);   // cyan target
            m_pTint->UpdateBuffer(tint);
            m_pTint->Bind();
        }

        m_pMesh->Draw();

        // Restore a clean slot 0 / VS / PS / cull state for whatever renders next.
        if (m_pNoCull) m_pNoCull->PostBind();
        ID3D11ShaderResourceView* pNull[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNull);
        pDC->VSSetShaderResources(0, 1, pNull);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }
}

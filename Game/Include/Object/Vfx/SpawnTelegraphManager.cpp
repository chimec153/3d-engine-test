#include "SpawnTelegraphManager.h"
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

namespace Client { SpawnTelegraphManager* SpawnTelegraphManager::m_pInst = nullptr; }

namespace Client
{
    namespace spawn_telegraph_detail
    {
        // ABGR (memory R,G,B,A) — matches the other VFX manager textures so
        // the same PS_UITint sampler reads alpha = texture.a * tint.a.
        constexpr unsigned int kRGB_White = (255u << 16) | (255u << 8) | 255u;

        inline unsigned int PackA(float a)
        {
            if (a < 0.f) a = 0.f; else if (a > 1.f) a = 1.f;
            return (static_cast<unsigned int>(a * 255.f) << 24) | kRGB_White;
        }
    }

    std::shared_ptr<Engine::Texture> SpawnTelegraphManager::EnsureRingTexture()
    {
        using namespace spawn_telegraph_detail;
        if (auto p = Engine::StaticFindBindable<Engine::Texture>("spawn_ring_tex"))
            return p;
        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("spawn_ring_tex");
        if (!pNew) return nullptr;

        // Bright annulus (outline only) so the player reads the spawn radius.
        // Empty interior — the fill disc lives in EnsureDiscTexture.
        constexpr int W = 64, H = 64;
        std::vector<unsigned int> buf(W * H);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const float u = (x + 0.5f) / W - 0.5f;
                const float v = (y + 0.5f) / H - 0.5f;
                const float r = std::sqrt(u * u + v * v) * 2.f;   // 0..1 across the quad
                float a = 0.f;
                if (r <= 1.f)
                {
                    // Triangular ring centred at r=0.9, half-width 0.12.
                    const float rim = 1.f - std::fabs(r - 0.9f) / 0.12f;
                    if (rim > 0.f) a = rim;
                }
                buf[y * W + x] = PackA(a);
            }
        }

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = buf.data();
        init.SysMemPitch = W * 4;
        pNew->CreateTexture(W, H, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
        pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        return pNew;
    }

    std::shared_ptr<Engine::Texture> SpawnTelegraphManager::EnsureDiscTexture()
    {
        using namespace spawn_telegraph_detail;
        if (auto p = Engine::StaticFindBindable<Engine::Texture>("spawn_disc_tex"))
            return p;
        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("spawn_disc_tex");
        if (!pNew) return nullptr;

        // Soft-edged solid disc — fades to 0 at the quad edge so the scaling
        // fill doesn't show pixel-grid aliasing while it grows.
        constexpr int W = 64, H = 64;
        std::vector<unsigned int> buf(W * H);
        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                const float u = (x + 0.5f) / W - 0.5f;
                const float v = (y + 0.5f) / H - 0.5f;
                const float r = std::sqrt(u * u + v * v) * 2.f;   // 0..1
                float a = 0.f;
                if (r <= 1.f)
                {
                    // Flat inside up to 0.85, smooth falloff to 1.0.
                    a = (r < 0.85f) ? 1.f : (1.f - (r - 0.85f) / 0.15f);
                }
                buf[y * W + x] = PackA(a);
            }
        }

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = buf.data();
        init.SysMemPitch = W * 4;
        pNew->CreateTexture(W, H, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
        pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        return pNew;
    }

    bool SpawnTelegraphManager::Init()
    {
        m_subs.clear();

        if (!m_bInit)
        {
            m_pRingTex = EnsureRingTexture();
            m_pDiscTex = EnsureDiscTexture();
            m_pVS   = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
            m_pPS   = Engine::StaticFindBindable<Engine::PixelShader> ("UIPSTint");
            m_pMesh = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
            m_pTopo = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
            m_pTint = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UITINTBUFFER>>("UITint");
            m_pUI   = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UICBUFFER>>("UI");
            m_pNoCull = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);

            m_pQuad = std::make_shared<Engine::Transform>();
            m_pQuad->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP for UIVS

            Engine::BindableRegistry::Register([]() { SpawnTelegraphManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pRingTex && m_pDiscTex && m_pVS && m_pPS && m_pMesh;
    }

    void SpawnTelegraphManager::Submit(const Engine::Vector3& vCentre, float fRadius, float fFill01)
    {
        if (fRadius <= 0.01f) return;
        if (fFill01 < 0.f) fFill01 = 0.f;
        if (fFill01 > 1.f) fFill01 = 1.f;
        m_subs.push_back({ vCentre, fRadius, fFill01 });
    }

    void SpawnTelegraphManager::Render()
    {
        if (!m_bInit || m_subs.empty()) return;
        if (!m_pVS || !m_pPS || !m_pMesh || !m_pQuad || !m_pRingTex || !m_pDiscTex) return;

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
        if (m_pTopo)   m_pTopo->Bind();
        if (m_pNoCull) m_pNoCull->Bind();
        // UIVS reads SV_VertexID, not VB inputs — clear the input layout so a
        // leftover IL from the opaque pass doesn't mismatch.
        pDC->IASetInputLayout(nullptr);
        Engine::Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;

        auto drawQuad = [&](const Engine::Vector3& vCentre, float fSize)
        {
            m_pQuad->SetScale(fSize, fSize, 1.f);
            m_pQuad->SetRX(-PI / 2.f);     // lay the XY quad onto the XZ ground
            m_pQuad->SetRY(0.f);
            m_pQuad->SetPosition(vCentre);
            m_pQuad->PostUpdate(0.f);
            const Engine::Vector3 ux = m_pQuad->GetAxis(Engine::AXIS_TYPE::X);
            const Engine::Vector3 uy = m_pQuad->GetAxis(Engine::AXIS_TYPE::Y);
            m_pQuad->SetPosition(vCentre - (ux + uy) * (0.5f * fSize));
            m_pQuad->PostUpdate(0.f);
            m_pQuad->Bind();
            m_pMesh->Draw();
        };

        auto bindTint = [&](float r, float g, float b, float a)
        {
            if (!m_pTint) return;
            Engine::UITINTBUFFER tint{};
            tint.vTint = Engine::Vector4(r, g, b, a);
            m_pTint->UpdateBuffer(tint);
            m_pTint->Bind();
        };

        // Pass 1: red rings at full radius — outlines pulse subtly so the player
        // sees the warning even at fFill01 near zero.
        m_pRingTex->Bind();
        for (const Tele& t : m_subs)
        {
            // Tint pulses red so the warning feels alive even before the fill
            // moves much. 0.6..0.95 alpha range.
            const float fPulse = 0.6f + 0.35f * t.fFill01;
            bindTint(1.0f, 0.15f, 0.15f, fPulse);
            drawQuad(t.vCentre, t.fRadius * 2.f);
        }

        // Pass 2: filled discs scaled by fFill01 — the "dot growing from centre".
        m_pDiscTex->Bind();
        for (const Tele& t : m_subs)
        {
            if (t.fFill01 <= 0.f) continue;
            // Fill alpha ramps up with the radius so the early dot is faint
            // and the near-complete fill is solid.
            const float fAlpha = 0.25f + 0.55f * t.fFill01;
            bindTint(1.0f, 0.25f, 0.20f, fAlpha);
            drawQuad(t.vCentre, t.fRadius * 2.f * t.fFill01);
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

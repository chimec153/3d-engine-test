#include "EnemyCountHUD.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Bindable/Texture.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/ConstantBuffer.h"
#include "Bindable/BindableManager.h"
#include "Render/RenderManager.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Types.h"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace Client
{
    namespace
    {
        // 5x7 monospace bitmap font for digits 0..9. Each row uses the
        // low 5 bits — bit 4 is the leftmost pixel, bit 0 the rightmost.
        // Atlas layout: 10 glyphs side-by-side, no padding.
        constexpr int kDigitW = 5;
        constexpr int kDigitH = 7;
        constexpr int kAtlasW = kDigitW * 10;
        constexpr int kAtlasH = kDigitH;

        const uint8_t kDigits[10][kDigitH] =
        {
            { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, // 0
            { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, // 1
            { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }, // 2
            { 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E }, // 3
            { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, // 4
            { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, // 5
            { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }, // 6
            { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, // 7
            { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, // 8
            { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }, // 9
        };

        // NDC layout (x: -1 left, +1 right ; y: -1 bottom, +1 top).
        constexpr float kBaseY      = 0.85f;   // baseline of top row (enemy count)
        constexpr float kRowGapY    = 0.10f;   // vertical spacing between rows
        constexpr float kRightEdge  = 0.95f;   // rightmost pixel of last digit
        constexpr float kDigitNDCW  = 0.045f;  // per-digit cell width
        constexpr float kDigitNDCH  = 0.08f;   // per-digit cell height
    }

    int EnemyCountHUD::CountEnemies() const
    {
        auto pScene = Engine::SceneManager::GetInst()->GetScene();
        if (!pScene) return 0;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return 0;

        int iCount = 0;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            // Tag match is intentional — the Editor loads the Game DLL at
            // runtime, so a dynamic_cast<Enemy*> would only work in builds
            // that already statically link Enemy. Filtering by IsActive
            // excludes corpses that have been InActivate()d this frame
            // but not yet pruned by the Scene.
            if (p && p->IsActive() && p->GetTag() == "Enemy")
                ++iCount;
        }
        return iCount;
    }

    void EnemyCountHUD::EnsureAtlas()
    {
        if (m_pAtlas) return;

        m_pAtlas = Engine::StaticFindBindable<Engine::Texture>("EnemyCountAtlas");
        if (m_pAtlas) return;

        // Build CPU-side RGBA buffer for the procedural 50x7 atlas.
        // Digit pixels = opaque white; background = fully transparent
        // (RenderUI binds AlphaBlend so the gap shows the scene behind).
        std::vector<uint32_t> pixels(kAtlasW * kAtlasH, 0x00000000u);
        for (int gx = 0; gx < 10; ++gx)
        {
            for (int row = 0; row < kDigitH; ++row)
            {
                const uint8_t rowBits = kDigits[gx][row];
                for (int col = 0; col < kDigitW; ++col)
                {
                    const bool bSet = (rowBits >> (kDigitW - 1 - col)) & 0x1;
                    if (!bSet) continue;
                    const int px = gx * kDigitW + col;
                    pixels[row * kAtlasW + px] = 0xFFFFFFFFu;
                }
            }
        }

        auto pNew = Engine::StaticCreateBindable<Engine::Texture>("EnemyCountAtlas");
        if (!pNew) return;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = pixels.data();
        init.SysMemPitch = kAtlasW * 4;
        pNew->CreateTexture(kAtlasW, kAtlasH, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
        pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
        m_pAtlas = pNew;
    }

    void EnemyCountHUD::DrawNumber(int iValue, float fBaseY)
    {
        // Break the integer into base-10 digits. Always show at least one
        // glyph (the zero case prints "0") and keep most-significant digit
        // first so the loop draws left-to-right.
        std::vector<int> digits;
        if (iValue <= 0)
        {
            digits.push_back(0);
        }
        else
        {
            int n = iValue;
            while (n > 0) { digits.push_back(n % 10); n /= 10; }
            // We built least-significant first; flip for draw order.
            std::reverse(digits.begin(), digits.end());
        }

        auto pTransformCB = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::TRANSFORMBUFFER>>("Transform");
        auto pUICB        = Engine::StaticFindBindable<Engine::ConstantBuffer<Engine::UICBUFFER>>("UI");
        if (!pTransformCB || !pUICB) return;

        const int   iNumDigits = static_cast<int>(digits.size());
        const float fBaseX     = kRightEdge - kDigitNDCW * iNumDigits;

        auto pDC = Engine::Graphics::GetInst()->GetDeviceContext();

        for (int i = 0; i < iNumDigits; ++i)
        {
            const int d = digits[i];

            // UV sub-range covering the d-th glyph in the atlas.
            Engine::UICBUFFER ucb{};
            ucb.vStartUV = { static_cast<float>(d)     / 10.f, 0.f };
            ucb.vEndUV   = { static_cast<float>(d + 1) / 10.f, 1.f };
            pUICB->UpdateBuffer(ucb);
            pUICB->Bind();

            // World-view-projection — same NDC convention as HPBar: scale
            // a unit quad, then translate to the digit's screen position.
            Engine::TRANSFORMBUFFER tbuf{};
            Engine::Matrix m =
                Engine::Matrix::Scaling({ kDigitNDCW, kDigitNDCH, 1.f }) *
                Engine::Matrix::TranslateFromVector(
                    { fBaseX + kDigitNDCW * i, fBaseY, 0.f });
            m.Transpose();
            tbuf.matWorldViewProject = m;
            pTransformCB->UpdateBuffer(tbuf);
            pTransformCB->Bind();

            // UI VS synthesises the four corners from SV_VertexID — no
            // VB/IB needed.
            pDC->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
            pDC->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
            pDC->IASetInputLayout(nullptr);
            pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            pDC->Draw(4, 0);
        }
    }

    void EnemyCountHUD::Render()
    {
        EnsureAtlas();
        if (!m_pAtlas) return;

        auto pVS = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
        auto pPS = Engine::StaticFindBindable<Engine::PixelShader> ("UIPS");
        if (!pVS || !pPS) return;

        pVS->Bind();
        pPS->Bind();
        m_pAtlas->Bind();

        // Row 0 (top): live enemy count.
        DrawNumber(CountEnemies(), kBaseY);

        // Rows below: one row per instanced bucket. Read at UI render
        // time, after RenderOpaque accumulated and before Clear() wipes
        // — see RenderManager::Render() ordering. The vector preserves
        // RenderOpaque's sort-by-(VS,PS,Material) order, so identical
        // entity types (e.g. all box enemies, all capsule enemies)
        // stay grouped into one row each.
        const auto& vecBuckets =
            Engine::RenderManager::GetInst()->GetInstancedBucketCounts();
        for (size_t i = 0; i < vecBuckets.size(); ++i)
        {
            const float fY = kBaseY - kRowGapY * static_cast<float>(i + 1);
            DrawNumber(vecBuckets[i], fY);
        }

        // Hard tear-down: unbind every GPU stage we touched so the next
        // frame's opaque pass starts from a known-clean state. Mirrors
        // HPBar's cleanup — t0 SRV / VS / PS staying bound past this
        // callback breaks downstream draws that share the BindCache.
        auto pDC = Engine::Graphics::GetInst()->GetDeviceContext();
        ID3D11ShaderResourceView* pNullSRV[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNullSRV);
        pDC->VSSetShaderResources(0, 1, pNullSRV);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);

        Engine::Graphics::GetInst()->ResetBindCache();
    }
}

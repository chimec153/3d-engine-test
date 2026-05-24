#include "Text.h"
#include "Font.h"
#include "FontManager.h"
#include "../Bindable/Transform.h"
#include "../Bindable/UIRenderer.h"
#include "../Bindable/Texture.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/BindableManager.h"
#include "../Core/Graphics.h"
#include "../Core/Window.h"
#include "../Render/RenderManager.h"
#include "../Types.h"
#include <d2d1.h>
#include <wincodec.h>
#include <cmath>
#include <vector>

namespace Engine
{
    namespace Text_detail
    {
        DWRITE_TEXT_ALIGNMENT MapH(Text::HAlign e)
        {
            switch (e)
            {
            case Text::HAlign::Left:   return DWRITE_TEXT_ALIGNMENT_LEADING;
            case Text::HAlign::Right:  return DWRITE_TEXT_ALIGNMENT_TRAILING;
            case Text::HAlign::Center:
            default:                   return DWRITE_TEXT_ALIGNMENT_CENTER;
            }
        }
        DWRITE_PARAGRAPH_ALIGNMENT MapV(Text::VAlign e)
        {
            switch (e)
            {
            case Text::VAlign::Top:    return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
            case Text::VAlign::Bottom: return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
            case Text::VAlign::Center:
            default:                   return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
            }
        }
    }

    Text::Text() : UIControl()
    {
        SetComponentType(COMPONENT_TYPE::NONE);
    }

    Text::Text(const Text& other)
        : UIControl(other)
        , m_pFont(other.m_pFont)
        , m_strText(other.m_strText)
        , m_uColorRGBA(other.m_uColorRGBA)
        , m_eHAlign(other.m_eHAlign), m_eVAlign(other.m_eVAlign)
        , m_bDirty(true)
        , m_fBoxX(other.m_fBoxX), m_fBoxY(other.m_fBoxY)
        , m_fBoxW(other.m_fBoxW), m_fBoxH(other.m_fBoxH)
    {
    }

    bool Text::Init()
    {
        if (!UIControl::Init()) return false;

        // Reuse the UIControl base m_pTransform as the tight-crop quad
        // placement. BakeToTexture rewrites its scale/position to the
        // glyph bounds.
        //
        // CRITICAL: detach from the parent Transform that UIControl::Init
        // auto-linked. The parent's PostUpdate-time scale (BoxW × BoxH for
        // LevelUpChoices et al) multiplies into ours and stretches the
        // glyph quad. Text places itself in absolute pixel coords, so
        // parent multiplication is unwanted.
        if (auto pTr = GetTransform()) pTr->SetParentTransform(nullptr);

        m_pRenderer = AddUIRenderer("text_renderer", nullptr);
        if (!m_pRenderer) return false;
        // PS_UITint — atlas-style: sample alpha from A8 texture, modulate
        // by the UITint cbuffer colour.
        if (auto pTintPS = StaticFindBindable<PixelShader>("UIPSTint"))
            m_pRenderer->SetPixelShader(pTintPS);

        m_pTintCBuffer =
            StaticFindBindable<ConstantBuffer<UITINTBUFFER>>("UITint");
        // UIControl::Init already populates the inherited m_pCBuffer
        // (b5 UI cbuffer) — PushUICBuffer mutates m_tCBuffer + binds.
        return true;
    }

    void Text::OnRectChanged(float fX, float fY, float fW, float fH)
    {
        // Capture box without touching the UIControl Transform — that
        // Transform is the quad placement, owned by BakeToTexture. Any
        // box change marks the layout dirty so the next PreDraw re-bakes
        // (DirectWrite wrapping + alignment depends on this box size).
        if (m_fBoxW != fW || m_fBoxH != fH) m_bDirty = true;
        m_fBoxX = fX; m_fBoxY = fY;
        m_fBoxW = fW; m_fBoxH = fH;
    }

    void Text::PreDraw(float fDeltaTime)
    {
        if (m_fBoxW > 0.f && m_fBoxH > 0.f &&
            (m_bDirty || m_fBoxW != m_fLastPxW || m_fBoxH != m_fLastPxH))
        {
            BakeToTexture(m_fBoxW, m_fBoxH);
        }

        // Queue a tint-push + UV-cbuffer reset callback BEFORE the child
        // UIRenderer registers its draw — AddCustomRender invokes
        // callbacks in registration order, so both cbuffers hold our
        // values at draw time. UV reset matters because a sibling like
        // EnemyCountHUD pushes per-glyph sub-regions into the same
        // b5 cbuffer that would otherwise crop our atlas.
        std::weak_ptr<Text> wpSelf = std::dynamic_pointer_cast<Text>(shared_from_this());
        RenderManager::GetInst()->AddCustomRender(RENDER_LAYER::UI,
            [wpSelf]()
            {
                if (auto pSelf = wpSelf.lock())
                {
                    pSelf->PushUICBuffer();
                    pSelf->PushTintColor();
                }
            });

        // Ticks child Components — UIRenderer's PreDraw self-registers
        // its Bind callback into the same UI render queue, after ours.
        UIControl::PreDraw(fDeltaTime);
    }

    void Text::SetFont(const std::shared_ptr<Font>& pFont)
    {
        if (m_pFont == pFont) return;
        m_pFont = pFont; m_bDirty = true;
    }
    void Text::SetString(const std::wstring& strText)
    {
        if (m_strText == strText) return;
        m_strText = strText; m_bDirty = true;
    }
    void Text::SetColor(unsigned int uRGBA)
    {
        // No re-bake — tint is applied per-frame via PushTintColor.
        m_uColorRGBA = uRGBA;
        PushTintColor();
    }
    void Text::SetTextureSize(int /*iWidth*/, int /*iHeight*/)
    {
        // No-op. The texture is now glyph-tight; the layout box comes
        // from SetRect + the live window size.
    }
    void Text::SetHAlign(HAlign e)
    {
        if (m_eHAlign == e) return;
        m_eHAlign = e; m_bDirty = true;
    }
    void Text::SetVAlign(VAlign e)
    {
        if (m_eVAlign == e) return;
        m_eVAlign = e; m_bDirty = true;
    }

    std::shared_ptr<Component> Text::Clone()
    {
        return std::make_shared<Text>(*this);
    }

    void Text::PushUICBuffer()
    {
        // Use the inherited UIControl b5 cbuffer (m_pCBuffer) so we
        // don't carry a duplicate handle. VS_UI multiplies the unit-
        // quad UV by (vEndUV-vStartUV)+vStartUV, so a full-quad atlas
        // sample needs (0,0)→(1,1).
        if (!m_pCBuffer) return;
        m_tCBuffer.vStartUV = Vector2(0.f, 0.f);
        m_tCBuffer.vEndUV   = Vector2(1.f, 1.f);
        m_pCBuffer->UpdateBuffer(m_tCBuffer);
        m_pCBuffer->Bind();
    }

    void Text::PushTintColor()
    {
        if (!m_pTintCBuffer) return;
        UITINTBUFFER buf{};
        // m_uColorRGBA is 0xRRGGBBAA. PS_UITint multiplies tint.rgb by
        // the master alpha; the blend state handles PMA on output.
        const float fA = ((m_uColorRGBA      ) & 0xFF) / 255.f;
        const float fR = ((m_uColorRGBA >> 24) & 0xFF) / 255.f;
        const float fG = ((m_uColorRGBA >> 16) & 0xFF) / 255.f;
        const float fB = ((m_uColorRGBA >>  8) & 0xFF) / 255.f;
        buf.vTint = Vector4(fR, fG, fB, fA);
        m_pTintCBuffer->UpdateBuffer(buf);
        m_pTintCBuffer->Bind();
    }

    bool Text::BakeToTexture(float fPxW, float fPxH)
    {
        // Reset previous bake so a failure path leaves the renderer with
        // no texture (renders nothing) rather than a stale glyph.
        m_pTextLayout.Reset();
        m_pBakedTex.reset();
        if (m_pRenderer) m_pRenderer->SetTexture(nullptr);

        if (m_strText.empty() || !m_pFont) { m_bDirty = false; return false; }
        IDWriteTextFormat* pFmt = m_pFont->GetTextFormat();
        if (!pFmt) return false;

        pFmt->SetTextAlignment     (Text_detail::MapH(m_eHAlign));
        pFmt->SetParagraphAlignment(Text_detail::MapV(m_eVAlign));
        pFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

        auto* pDWrite = FontManager::GetInst()->GetDWriteFactory();
        auto* pD2D    = FontManager::GetInst()->GetD2DFactory();
        auto* pWIC    = FontManager::GetInst()->GetWICFactory();
        if (!pDWrite || !pD2D || !pWIC) return false;

        // Build the layout at the requested box size — DirectWrite
        // resolves wrapping + alignment against this rectangle.
        if (FAILED(pDWrite->CreateTextLayout(
                m_strText.c_str(), static_cast<UINT32>(m_strText.size()),
                pFmt, fPxW, fPxH,
                m_pTextLayout.GetAddressOf())) || !m_pTextLayout)
            return false;

        // Tight-crop to the actual painted glyph bounds — saves the
        // empty padding inside the layout box. GetMetrics gives the
        // *content* extents in DIP; we round outward to pixels so no
        // glyph edge gets clipped by AA.
        DWRITE_TEXT_METRICS tm = {};
        if (FAILED(m_pTextLayout->GetMetrics(&tm))) return false;

        const float fLeftDip = tm.left;
        const float fTopDip  = tm.top;
        const int   iCropW   = static_cast<int>(std::ceil(tm.width));
        const int   iCropH   = static_cast<int>(std::ceil(tm.height));
        if (iCropW <= 0 || iCropH <= 0) { m_bDirty = false; return false; }

        // BGRA WIC bitmap — D2D's only widely-supported pixel format for
        // CreateWicBitmapRenderTarget. We strip everything but alpha into
        // an A8 texture below.
        Microsoft::WRL::ComPtr<IWICBitmap> pWicBmp;
        if (FAILED(pWIC->CreateBitmap(
                static_cast<UINT>(iCropW), static_cast<UINT>(iCropH),
                GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad,
                pWicBmp.GetAddressOf())) || !pWicBmp)
            return false;

        const D2D1_RENDER_TARGET_PROPERTIES rtProps =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED));
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> pRT;
        if (FAILED(pD2D->CreateWicBitmapRenderTarget(
                pWicBmp.Get(), rtProps, pRT.GetAddressOf())) || !pRT)
            return false;
        pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

        // White brush — colour comes from the tint cbuffer at sample
        // time, so the baked texture stores coverage only.
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrush;
        pRT->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 1.f, 1.f, 1.f), pBrush.GetAddressOf());
        if (!pBrush) return false;

        pRT->BeginDraw();
        pRT->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));   // start fully transparent
        // Shift the layout so the crop's (left, top) ends up at (0, 0)
        // in the bitmap.
        pRT->DrawTextLayout(
            D2D1::Point2F(-fLeftDip, -fTopDip),
            m_pTextLayout.Get(), pBrush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_NONE);
        if (FAILED(pRT->EndDraw())) return false;

        // Pull the BGRA pixels back to system memory and reduce to A8.
        const UINT uPitchBgra = static_cast<UINT>(iCropW) * 4u;
        std::vector<unsigned char> bgra(static_cast<size_t>(uPitchBgra) * iCropH);
        const WICRect rect = { 0, 0, iCropW, iCropH };
        if (FAILED(pWicBmp->CopyPixels(&rect, uPitchBgra,
                static_cast<UINT>(bgra.size()), bgra.data())))
            return false;

        std::vector<unsigned char> a8(static_cast<size_t>(iCropW) * iCropH);
        for (size_t i = 0; i < a8.size(); ++i)
            a8[i] = bgra[i * 4 + 3];   // alpha = 4th byte of PBGRA

        // Upload to a fresh A8 GPU texture. shared_ptr ownership; the
        // old m_pBakedTex's D3D resources are released on assignment.
        auto pTex = std::make_shared<Texture>();
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = a8.data();
        init.SysMemPitch = static_cast<UINT>(iCropW);
        if (!pTex->CreateTexture(iCropW, iCropH,
                DXGI_FORMAT_A8_UNORM, 1, 1, &init))
            return false;
        if (!pTex->CreateShaderResourceView(DXGI_FORMAT_A8_UNORM, 1, 1))
            return false;

        m_pBakedTex = pTex;
        if (m_pRenderer) m_pRenderer->SetTexture(m_pBakedTex);

        // Rewrite the UIControl base m_pTransform to the tight-crop quad
        // placement. UIRenderer's target is this same Transform, so the
        // next draw uses these values directly.
        if (auto pTr = GetTransform())
        {
            pTr->SetPosition(m_fBoxX + fLeftDip, m_fBoxY + fTopDip, 0.f);
            pTr->SetScale(static_cast<float>(iCropW),
                          static_cast<float>(iCropH), 1.f);

            // BakeToTexture runs from PreDraw, which is AFTER this frame's
            // PostUpdate has already built m_tBuffer.matWorldViewProject
            // from the *previous* scale. Without an explicit PostUpdate
            // here the very next Bind would push that stale matrix —
            // observed as a stretched quad sized to the layout box rather
            // than the tight crop. Forcing PostUpdate rebuilds the UI
            // matrix from the scale we just wrote.
            pTr->PostUpdate(0.f);
        }

        m_fLastPxW = fPxW;
        m_fLastPxH = fPxH;
        m_bDirty   = false;

        // Defensive: push tint + UV cbuffer right after the first
        // successful bake so initial values show up even if the
        // per-frame callback dispatch hasn't fired yet (first Render).
        PushUICBuffer();
        PushTintColor();
        return true;
    }
}

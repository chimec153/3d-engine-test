#include "Text.h"
#include "Font.h"
#include "FontManager.h"
#include "../Core/Graphics.h"
#include "../Core/Window.h"
#include "../Render/RenderManager.h"
#include <Windows.h>
#include <dxgi.h>

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
        , m_fNdcX(other.m_fNdcX), m_fNdcY(other.m_fNdcY)
        , m_fNdcW(other.m_fNdcW), m_fNdcH(other.m_fNdcH)
        , m_eHAlign(other.m_eHAlign), m_eVAlign(other.m_eVAlign)
        , m_bLayoutDirty(true)
    {
    }

    bool Text::Init()
    {
        // Text doesn't own a UIRenderer or Transform — the back-buffer
        // D2D path handles its own placement. UIControl::Init still
        // sets up the parent-side cbuffer machinery in case future
        // subclasses use it; harmless here.
        return UIControl::Init();
    }

    void Text::PreDraw(float fDeltaTime)
    {
        UIControl::PreDraw(fDeltaTime);

        // Register a UI-pass render callback every frame. RenderManager
        // clears m_CustomRenderList per-frame, so this stays in sync
        // with active components (a disabled Text simply doesn't
        // PreDraw — Component::PreDraw skips inactive children).
        std::weak_ptr<Text> wpSelf = std::dynamic_pointer_cast<Text>(shared_from_this());
        RenderManager::GetInst()->AddCustomRender(RENDER_LAYER::UI,
            [wpSelf]()
            {
                if (auto pSelf = wpSelf.lock()) pSelf->RenderD2D();
            });
    }

    void Text::SetRect(float fX, float fY, float fW, float fH)
    {
        if (m_fNdcX == fX && m_fNdcY == fY && m_fNdcW == fW && m_fNdcH == fH) return;
        m_fNdcX = fX; m_fNdcY = fY; m_fNdcW = fW; m_fNdcH = fH;
        m_bLayoutDirty = true;
    }

    void Text::SetFont(const std::shared_ptr<Font>& pFont)
    {
        if (m_pFont == pFont) return;
        m_pFont = pFont; m_bLayoutDirty = true;
    }
    void Text::SetString(const std::wstring& strText)
    {
        if (m_strText == strText) return;
        m_strText = strText; m_bLayoutDirty = true;
    }
    void Text::SetColor(unsigned int uRGBA)
    {
        // Colour only — no layout rebuild.
        m_uColorRGBA = uRGBA;
    }
    void Text::SetTextureSize(int /*iWidth*/, int /*iHeight*/)
    {
        // No-op. Kept for API stability; the live layout box now comes
        // straight from SetRect + the current window dimensions.
    }
    void Text::SetHAlign(HAlign e)
    {
        if (m_eHAlign == e) return;
        m_eHAlign = e; m_bLayoutDirty = true;
    }
    void Text::SetVAlign(VAlign e)
    {
        if (m_eVAlign == e) return;
        m_eVAlign = e; m_bLayoutDirty = true;
    }

    std::shared_ptr<Component> Text::Clone()
    {
        return std::make_shared<Text>(*this);
    }

    bool Text::BuildLayout(float fPxW, float fPxH)
    {
        m_pTextLayout.Reset();

        if (!m_pFont) return false;
        IDWriteTextFormat* pFmt = m_pFont->GetTextFormat();
        if (!pFmt) return false;

        pFmt->SetTextAlignment     (Text_detail::MapH(m_eHAlign));
        pFmt->SetParagraphAlignment(Text_detail::MapV(m_eVAlign));
        pFmt->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

        auto* pDWrite = FontManager::GetInst()->GetDWriteFactory();
        if (!pDWrite) return false;

        HRESULT hr = pDWrite->CreateTextLayout(
            m_strText.c_str(), static_cast<UINT32>(m_strText.size()),
            pFmt, fPxW, fPxH,
            m_pTextLayout.GetAddressOf());
        if (FAILED(hr) || !m_pTextLayout) return false;

        m_bLayoutDirty = false;
        m_fLastPxW = fPxW;
        m_fLastPxH = fPxH;
        return true;
    }

    void Text::RenderD2D()
    {
        if (m_strText.empty() || !m_pFont) return;

        // NDC → backbuffer pixel rect. NDC X spans [-1,+1] across the
        // window width; NDC Y is bottom-up so flip to D2D's top-down
        // pixel space.
        const float fW = static_cast<float>(Window::GetInst()->GetWidth());
        const float fH = static_cast<float>(Window::GetInst()->GetHeight());
        if (fW <= 0.f || fH <= 0.f) return;

        const float fPxX = (m_fNdcX + 1.f)             * 0.5f * fW;
        const float fPxY = (1.f - (m_fNdcY + m_fNdcH)) * 0.5f * fH;
        const float fPxW = m_fNdcW * 0.5f * fW;
        const float fPxH = m_fNdcH * 0.5f * fH;
        if (fPxW <= 0.f || fPxH <= 0.f) return;

        // Rebuild the layout when the box dimensions change (window
        // resize, SetRect) or any input is dirty.
        if (m_bLayoutDirty || fPxW != m_fLastPxW || fPxH != m_fLastPxH)
        {
            if (!BuildLayout(fPxW, fPxH)) return;
        }
        if (!m_pTextLayout) return;

        // Wrap the current back-buffer (buffer 0) in a D2D RT. With
        // DXGI_SWAP_EFFECT_FLIP_DISCARD the surface may be a different
        // resource each Present, so we re-create the RT every frame.
        // Cost is small relative to a typical UI frame.
        const auto& pSwap = Graphics::GetInst()->GetSwapChain();
        if (!pSwap) return;
        Microsoft::WRL::ComPtr<IDXGISurface> pSurface;
        if (FAILED(pSwap->GetBuffer(0, __uuidof(IDXGISurface),
                reinterpret_cast<void**>(pSurface.GetAddressOf()))))
            return;

        auto* pD2D = FontManager::GetInst()->GetD2DFactory();
        if (!pD2D) return;

        const D2D1_RENDER_TARGET_PROPERTIES rtProps =
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED));
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> pRT;
        if (FAILED(pD2D->CreateDxgiSurfaceRenderTarget(
                pSurface.Get(), rtProps, pRT.GetAddressOf())))
            return;
        pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

        // Premultiplied alpha for the PMA target.
        const float fA = ((m_uColorRGBA      ) & 0xFF) / 255.f;
        const float fR = ((m_uColorRGBA >> 24) & 0xFF) / 255.f * fA;
        const float fG = ((m_uColorRGBA >> 16) & 0xFF) / 255.f * fA;
        const float fB = ((m_uColorRGBA >>  8) & 0xFF) / 255.f * fA;

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBrush;
        pRT->CreateSolidColorBrush(
            D2D1::ColorF(fR, fG, fB, fA), pBrush.GetAddressOf());
        if (!pBrush) return;

        pRT->BeginDraw();
        // No Clear — we're drawing on top of the existing back-buffer
        // contents (the rest of the UI pass already drew there).
        pRT->DrawTextLayout(
            D2D1::Point2F(fPxX, fPxY),
            m_pTextLayout.Get(), pBrush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_NONE);
        pRT->EndDraw();
    }
}

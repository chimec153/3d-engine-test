#pragma once

#include "../Core/Macro.h"
#include "../UI/UIControl.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <memory>
#include <string>

namespace Engine
{
    class Font;

    // UI text component that paints itself straight onto the back-
    // buffer with Direct2D + DirectWrite. No intermediate texture, no
    // UIRenderer — Text registers a custom render callback into the
    // RenderManager's UI pass via PreDraw, and the callback wraps the
    // current back-buffer surface in a transient ID2D1RenderTarget for
    // a single DrawTextLayout call.
    //
    // Usage:
    //   auto pText = pOwnerUIControl->CreateComponent<Text>("name");
    //   pText->SetFont(pFont);
    //   pText->SetRect(x, y, w, h);    // NDC placement, like Button
    //   pText->SetString(L"Arrow");
    //   pText->SetColor(0xFFFFFFFFu);
    class ENGINE_DLL Text : public UIControl
    {
    public:
        Text();
        Text(const Text& other);
        virtual ~Text() override = default;

        // NDC placement — bottom-left (fX, fY), size (fW, fH). The
        // rectangle is converted to back-buffer pixels at paint time
        // so the same NDC values look identical on any window size.
        void SetRect(float fX, float fY, float fW, float fH);

        void SetFont (const std::shared_ptr<Font>& pFont);
        void SetString(const std::wstring& strText);
        // 0xRRGGBBAA.
        void SetColor(unsigned int uRGBA);
        // Compatibility hook. The new render path computes the layout
        // box from SetRect's NDC dimensions and the live window size,
        // so this value is currently ignored.
        void SetTextureSize(int iWidth, int iHeight);

        enum class HAlign { Left, Center, Right };
        enum class VAlign { Top,  Center, Bottom };
        void SetHAlign(HAlign e);
        void SetVAlign(VAlign e);

        const std::wstring&        GetString() const { return m_strText; }
        const std::shared_ptr<Font>& GetFont() const { return m_pFont; }

        virtual bool Init() override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        // (re)build m_pTextLayout for the current (string, font,
        // alignment, layout box). Called from RenderD2D when the
        // layout box or any input has changed.
        bool BuildLayout(float fPxW, float fPxH);
        // RenderManager's UI pass invokes this after the D3D11 UI
        // sub-passes — wraps the back-buffer in a transient D2D RT
        // and issues one DrawTextLayout call.
        void RenderD2D();

        std::shared_ptr<Font> m_pFont;
        std::wstring          m_strText;
        unsigned int          m_uColorRGBA = 0xFFFFFFFFu;

        // NDC rect from SetRect. Y is bottom-up (NDC convention) —
        // RenderD2D flips to D2D's top-down screen coords.
        float                 m_fNdcX = 0.f;
        float                 m_fNdcY = 0.f;
        float                 m_fNdcW = 0.f;
        float                 m_fNdcH = 0.f;

        HAlign                m_eHAlign = HAlign::Center;
        VAlign                m_eVAlign = VAlign::Center;
        bool                  m_bLayoutDirty = true;

        // Cached DWrite layout. Rebuilt only when the layout box or
        // any input changes — the per-frame DrawTextLayout call just
        // reuses this object.
        Microsoft::WRL::ComPtr<IDWriteTextLayout> m_pTextLayout;
        float                 m_fLastPxW = 0.f;
        float                 m_fLastPxH = 0.f;
    };
}

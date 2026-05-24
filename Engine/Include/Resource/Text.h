#pragma once

#include "../Core/Macro.h"
#include "../UI/UIControl.h"
#include <dwrite.h>
#include <wrl/client.h>
#include <memory>
#include <string>

namespace Engine
{
    class Font;
    class Texture;
    class UIRenderer;
    class Transform;
    template <typename T> class ConstantBuffer;
    struct _tagUITintBuffer;

    // UI text component. DirectWrite computes the layout (kerning,
    // alignment, line breaking, font fallback); the result is baked once
    // into an A8 texture and drawn each frame as a single UIRenderer
    // quad — same path HPBar/Button use. No per-frame D2D BeginDraw,
    // no swap-chain RT churn.
    //
    // Re-bakes only when the inputs change (SetString / SetFont /
    // alignment / box size). Steady-state cost = one textured quad.
    //
    // Memory: baked texture is A8 (alpha-only) and tight-cropped to the
    // actual glyph bounds — ~8x less than the previous BGRA whole-box
    // bitmap. The colour is applied via the UITint cbuffer in PS_UITint.
    //
    // Usage:
    //   auto pText = pOwnerUIControl->CreateComponent<Text>("name");
    //   pText->SetFont(pFont);
    //   pText->SetRect(x, y, w, h);
    //   pText->SetString(L"Arrow");
    //   pText->SetColor(0xFFFFFFFFu);
    class ENGINE_DLL Text : public UIControl
    {
    public:
        Text();
        Text(const Text& other);
        virtual ~Text() override = default;

        // SetRect inherited from UIControl — pixel coords, (x, y)
        // top-left, (w, h) layout box size. The actual baked texture is
        // tight-cropped to the glyph bounds inside this box; the box
        // still drives alignment / wrapping via DirectWrite.
        void SetFont (const std::shared_ptr<Font>& pFont);
        void SetString(const std::wstring& strText);
        // 0xRRGGBBAA. Applied via the UITint cbuffer — no re-bake.
        void SetColor(unsigned int uRGBA);
        // Compatibility hook; layout box now comes from SetRect.
        void SetTextureSize(int iWidth, int iHeight);

        enum class HAlign { Left, Center, Right };
        enum class VAlign { Top,  Center, Bottom };
        void SetHAlign(HAlign e);
        void SetVAlign(VAlign e);

        const std::wstring&         GetString() const { return m_strText; }
        const std::shared_ptr<Font>& GetFont()  const { return m_pFont; }

        virtual bool Init() override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    protected:
        // Capture SetRect input into m_fBoxW/H rather than letting the
        // base set the UIControl Transform scale to the box — we reuse
        // the UIControl Transform as the *tight-crop* quad placement
        // (BakeToTexture rewrites scale/position to glyph bounds).
        virtual void OnRectChanged(float fX, float fY, float fW, float fH) override;

    private:
        // Build the DirectWrite layout for the current box, measure
        // tight glyph bounds (GetMetrics), render once into an A8
        // texture, point the UIRenderer at it, and rewrite the
        // UIControl Transform to place the tight-cropped quad.
        bool BakeToTexture(float fPxW, float fPxH);

        // Re-applies the current tint colour into the UITint cbuffer.
        // Called via the per-frame UI custom render callback (so the
        // cbuffer holds our value at the exact moment the draw fires).
        void PushTintColor();

        // Pushes a full-quad (0,0)-(1,1) sub-region into the shared b5
        // UI cbuffer. VS_UI multiplies the unit-quad UV by
        // (vEndUV-vStartUV)+vStartUV; without our push the previous
        // draw's sub-region (e.g. EnemyCountHUD's last digit cell)
        // would crop our atlas. Runs from the same per-frame UI
        // callback as PushTintColor.
        void PushUICBuffer();

        std::shared_ptr<Font>        m_pFont;
        std::wstring                 m_strText;
        unsigned int                 m_uColorRGBA = 0xFFFFFFFFu;

        HAlign                       m_eHAlign = HAlign::Center;
        VAlign                       m_eVAlign = VAlign::Center;
        bool                         m_bDirty  = true;

        // Layout box — captured from SetRect / anchor-recompute via
        // OnRectChanged. Drives DirectWrite's layout box (for wrapping
        // and alignment); the actual quad transform is the UIControl
        // base m_pTransform, rewritten by BakeToTexture to tight crop.
        float                        m_fBoxX = 0.f;
        float                        m_fBoxY = 0.f;
        float                        m_fBoxW = 0.f;
        float                        m_fBoxH = 0.f;

        // Cached DWrite layout — rebuilt only when an input changes.
        Microsoft::WRL::ComPtr<IDWriteTextLayout> m_pTextLayout;
        float                        m_fLastPxW = 0.f;
        float                        m_fLastPxH = 0.f;

        // Child UIRenderer wired in Init via UIControl::AddUIRenderer.
        // Target is the UIControl base m_pTransform — we reuse that
        // Transform as the tight-crop quad placement.
        std::shared_ptr<UIRenderer>  m_pRenderer;
        std::shared_ptr<Texture>     m_pBakedTex;

        // Shared UITint cbuffer (registered by BindableManager). We
        // upload the current colour into it before the PS samples.
        // The b5 UI cbuffer comes from UIControl::m_pCBuffer (base) —
        // PushUICBuffer mutates UIControl::m_tCBuffer and binds it.
        std::shared_ptr<ConstantBuffer<_tagUITintBuffer>> m_pTintCBuffer;
    };
}

#pragma once

#include "UIControl.h"
#include <functional>
#include <memory>

namespace Engine
{
    class Slider;
    class EditBox;
    class Font;

    // A numeric input row composed of two standalone controls: a Slider (left,
    // drag-only) and an EditBox (right, click-to-type). They stay in sync — a
    // drag updates the box text, a keystroke moves the handle — and the value
    // is clamped to [min,max] / rounded to the decimal count. The two children
    // do all the drawing and interaction; NumberField only lays them out within
    // its field rect and wires their callbacks together.
    //
    // Usage:
    //   auto f = host->CreateComponent<Engine::NumberField>("life");
    //   f->SetFont(pFont);
    //   f->SetRange(0.1f, 10.f);
    //   f->SetDecimals(1);
    //   f->SetFieldRect(x, y, w, h);
    //   f->SetValue(2.0f);
    //   f->SetOnChange([this](float){ RefreshCraft(); });
    class ENGINE_DLL NumberField : public UIControl
    {
    public:
        NumberField();
        NumberField(const NumberField& other) = default;
        virtual ~NumberField() override = default;

        void SetFieldRect(float fX, float fY, float fW, float fH);
        void SetFont(const std::shared_ptr<Font>& pFont);
        void SetRange(float fMin, float fMax);
        void SetDecimals(int iDecimals);
        void SetValue(float fValue);
        float GetValue() const { return m_fValue; }
        void SetEnabled(bool bEnabled);
        void SetOnChange(std::function<void(float)> fn) { m_fnOnChange = std::move(fn); }

        virtual bool Init() override;
        virtual std::shared_ptr<Component> Clone() override;

    protected:
        // Follow SetRect (e.g. when a ScrollView re-places this field) by
        // laying the slider + edit box out within the new rect. Without this a
        // NumberField placed in a ScrollView wouldn't move when scrolled.
        virtual void OnRectChanged(float fX, float fY, float fW, float fH) override;

    private:
        float ClampRound(float fValue) const;

        std::shared_ptr<Slider>  m_pSlider;
        std::shared_ptr<EditBox> m_pBox;

        float m_fMin      = 0.f;
        float m_fMax      = 1.f;
        float m_fValue    = 0.f;
        int   m_iDecimals = 1;

        std::function<void(float)> m_fnOnChange;
    };
}

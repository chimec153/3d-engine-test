#include "NumberField.h"
#include "Slider.h"
#include "EditBox.h"
#include <cmath>

namespace Engine
{
    NumberField::NumberField()
        : UIControl()
    {
        Component::SetComponentType(COMPONENT_TYPE::NONE);
    }

    bool NumberField::Init()
    {
        if (!UIControl::Init()) return false;

        m_pSlider = CreateComponent<Slider>("nf_slider");
        m_pBox    = CreateComponent<EditBox>("nf_box");

        // Slider drag -> clamp/round, sync the box, report the value.
        if (m_pSlider)
        {
            m_pSlider->SetOnChange([this](float v)
            {
                const float vc = ClampRound(v);
                m_fValue = vc;
                if (m_pBox)    m_pBox->SetValue(vc);     // silent — no feedback loop
                if (m_pSlider) m_pSlider->SetValue(vc);  // snap the handle to the rounded value
                if (m_fnOnChange) m_fnOnChange(vc);
            });
        }

        if (m_pBox)
        {
            // Live keystroke: move the handle to the clamped value but report
            // the raw typed value (so a preview updates as you type); don't
            // re-format the box mid-typing.
            m_pBox->SetOnChange([this](float v)
            {
                m_fValue = v;
                if (m_pSlider) m_pSlider->SetValue(ClampRound(v));
                if (m_fnOnChange) m_fnOnChange(v);
            });
            // Commit (Enter / blur): clamp+round and re-format both controls.
            m_pBox->SetOnCommit([this](float v)
            {
                const float vc = ClampRound(v);
                m_fValue = vc;
                if (m_pBox)    m_pBox->SetValue(vc);
                if (m_pSlider) m_pSlider->SetValue(vc);
                if (m_fnOnChange) m_fnOnChange(vc);
            });
        }
        return true;
    }

    float NumberField::ClampRound(float fValue) const
    {
        float v = fValue < m_fMin ? m_fMin : (fValue > m_fMax ? m_fMax : fValue);
        if (m_iDecimals >= 0)
        {
            const float f = std::pow(10.f, static_cast<float>(m_iDecimals));
            v = std::round(v * f) / f;
        }
        return v;
    }

    void NumberField::OnRectChanged(float fX, float fY, float fW, float fH)
    {
        // SetRect (ScrollView re-placement, window resize) → re-lay children.
        SetFieldRect(fX, fY, fW, fH);
    }

    void NumberField::SetFieldRect(float fX, float fY, float fW, float fH)
    {
        // Slider takes the left ~58%, the edit box the right ~38%, with a gap.
        const float fSliderW = fW * 0.58f;
        const float fBoxX    = fX + fW * 0.62f;
        const float fBoxW    = fW * 0.38f;
        if (m_pSlider) m_pSlider->SetSliderRect(fX, fY, fSliderW, fH);
        if (m_pBox)    m_pBox->SetBoxRect(fBoxX, fY, fBoxW, fH);
    }

    void NumberField::SetFont(const std::shared_ptr<Font>& pFont)
    {
        if (m_pBox) m_pBox->SetFont(pFont);
    }

    void NumberField::SetRange(float fMin, float fMax)
    {
        m_fMin = fMin; m_fMax = fMax;
        if (m_pSlider) m_pSlider->SetRange(fMin, fMax);
        if (m_pBox)    m_pBox->SetAllowNegative(fMin < 0.f);
    }

    void NumberField::SetDecimals(int iDecimals)
    {
        m_iDecimals = iDecimals;
        if (m_pBox) m_pBox->SetDecimals(iDecimals);
    }

    void NumberField::SetValue(float fValue)
    {
        const float vc = ClampRound(fValue);
        m_fValue = vc;
        if (m_pSlider) m_pSlider->SetValue(vc);
        if (m_pBox)    m_pBox->SetValue(vc);
    }

    void NumberField::SetEnabled(bool bEnabled)
    {
        if (m_pSlider) m_pSlider->SetEnabled(bEnabled);
        if (m_pBox)    m_pBox->SetEnabled(bEnabled);
    }

    std::shared_ptr<Component> NumberField::Clone()
    {
        return std::make_shared<NumberField>(*this);
    }
}

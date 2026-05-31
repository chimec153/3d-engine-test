#pragma once

#include "UIControl.h"
#include <functional>
#include <memory>

namespace Engine
{
    class Button;

    // A horizontal slider control: a groove (track) plus a draggable handle.
    // Drag the handle or click anywhere on the track to set a value in
    // [min,max]. Draws via two child Button quads; interaction is polled in
    // Update from CInput (its own base Transform is never placed, so the base
    // never hit-tests it — same approach as ScrollView).
    //
    // SetValue() is silent (no callback) so an owner can sync it without
    // feedback loops; only user drags fire SetOnChange.
    class ENGINE_DLL Slider : public UIControl
    {
    public:
        Slider();
        Slider(const Slider& other) = default;
        virtual ~Slider() override = default;

        // Placement in screen pixels (own rect; not the base Transform).
        void SetSliderRect(float fX, float fY, float fW, float fH);
        void SetRange(float fMin, float fMax) { m_fMin = fMin; m_fMax = fMax; PlaceHandle(); }
        void SetValue(float fValue);                 // silent: positions the handle, no callback
        float GetValue() const { return m_fValue; }
        void SetEnabled(bool bEnabled);
        void SetOnChange(std::function<void(float)> fn) { m_fnOnChange = std::move(fn); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        void PlaceHandle();              // position the handle from the value
        void SetFromMouseX(float fMouseX); // user drag: set value + fire onChange

        std::shared_ptr<Button> m_pTrack;
        std::shared_ptr<Button> m_pHandle;

        float m_fRect[4]       = { 0.f, 0.f, 0.f, 0.f };  // x, y, w, h
        float m_fTrackRect[4]  = { 0.f, 0.f, 0.f, 0.f };
        float m_fHandleRect[4] = { 0.f, 0.f, 0.f, 0.f };

        float m_fMin   = 0.f;
        float m_fMax   = 1.f;
        float m_fValue = 0.f;

        bool  m_bEnabled  = true;
        bool  m_bDragging = false;

        std::function<void(float)> m_fnOnChange;
    };
}

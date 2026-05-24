#pragma once

#include "UIControl.h"
#include <functional>
#include <memory>

namespace Engine
{
    class Transform;
    class UIRenderer;
    class Texture;

    // Click-aware UI primitive. Same UIControl + child UIRenderer + child
    // Transform composition as HPBar/XPBar, but exposes a click rect in
    // NDC and a std::function callback. Update polls the engine's mouse
    // state every frame; a left-button-down whose NDC coords fall inside
    // the rect fires OnClick once. Pausing the game (Window::Stop) does
    // NOT silence the button — input state is polled regardless of the
    // time-scale gate so callers can use buttons inside paused modals.
    class ENGINE_DLL Button : public UIControl
    {
    public:
        Button();
        virtual ~Button() override = default;

        // SetRect is inherited from UIControl — pixel coords, (x,y) is
        // the top-left, (w,h) the size. Button stores no rect of its
        // own; placement lives on the UIControl-owned Transform.
        void SetTexture(const std::shared_ptr<Texture>& pTex);
        void SetOnClick(std::function<void()> fnOnClick) { m_fnOnClick = std::move(fnOnClick); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        // Test whether the mouse cursor (window pixels) falls inside
        // the UIControl Transform's pixel rect.
        bool HitTestMousePx() const;

        std::function<void()> m_fnOnClick;

        std::shared_ptr<UIRenderer> m_pRenderer;
    };
}

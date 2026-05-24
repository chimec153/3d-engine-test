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
    // Transform composition as HPBar/XPBar; the click handler is wired
    // through UIControl::OnMouseDown — the base polls input + hit-tests
    // every frame, and Button's override fires the callback. Pausing the
    // game (Timer::Stop) does NOT silence the button, since the base's
    // poll runs regardless of the time-scale gate.
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
        virtual std::shared_ptr<Component> Clone() override;

    protected:
        virtual void OnMouseDown() override;

    private:
        std::function<void()> m_fnOnClick;

        std::shared_ptr<UIRenderer> m_pRenderer;
    };
}

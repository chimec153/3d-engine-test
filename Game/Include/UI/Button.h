#pragma once

#include "UI/UIControl.h"
#include "../GameDefs.h"
#include <functional>
#include <memory>

namespace Engine
{
    class Transform;
    class UIRenderer;
    class Texture;
}

namespace Client
{
    // Click-aware UI primitive. Same UIControl + child UIRenderer + child
    // Transform composition as HPBar/XPBar, but exposes a click rect in
    // NDC and a std::function callback. Update polls the engine's mouse
    // state every frame; a left-button-down whose NDC coords fall inside
    // the rect fires OnClick once. Pausing the game (Window::Stop) does
    // NOT silence the button — input state is polled regardless of the
    // time-scale gate so callers can use buttons inside paused modals.
    //
    // Game-side placement (rather than Engine::UIControl-sibling) keeps
    // it close to its only current consumer (LevelUpChoices). Promote
    // to Engine/Include/UI if a second UI subsystem needs it.
    class GAME_DLL Button : public Engine::UIControl
    {
    public:
        Button();
        virtual ~Button() override = default;

        // NDC rect — (x, y) is the bottom-left corner, (w, h) the size.
        void SetRect(float fX, float fY, float fW, float fH);
        void SetTexture(const std::shared_ptr<Engine::Texture>& pTex);
        void SetOnClick(std::function<void()> fnOnClick) { m_fnOnClick = std::move(fnOnClick); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        bool HitTestMouseNDC() const;

        std::function<void()> m_fnOnClick;
        float m_fX = 0.f;
        float m_fY = 0.f;
        float m_fW = 0.f;
        float m_fH = 0.f;

        std::shared_ptr<Engine::Transform>  m_pTransform;
        std::shared_ptr<Engine::UIRenderer> m_pRenderer;
    };
}

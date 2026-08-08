#pragma once

#include "UI/UIControl.h"
#include "Core/Macro.h"
#include <memory>

namespace Engine
{
    class Font;
    class Text;
    class Button;
}

namespace Client
{
    class Player;

    // Bottom-left HUD: a single button that shows the current aim mode
    // (LCTRL target / mouse-aim toggle) and flips it when clicked — exactly
    // like pressing Ctrl. Polls Player::IsMouseAim() each frame so the label
    // and colour stay in sync no matter which input (key or button) toggled
    // it. Same UIControl + Button + Text composition as PauseMenuUI.
    class GAME_DLL AimModeButtonUI : public Engine::UIControl
    {
    public:
        AimModeButtonUI();
        virtual ~AimModeButtonUI() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        void ApplyState(bool bOn);   // push label + colour for the given mode

        std::weak_ptr<Player>           m_pTarget;
        std::shared_ptr<Engine::Font>   m_pFont;
        std::shared_ptr<Engine::Font>   m_pHintFont;
        std::shared_ptr<Engine::Button> m_pButton;
        std::shared_ptr<Engine::Text>   m_pLabel;
        std::shared_ptr<Engine::Text>   m_pHint;    // key-press caption

        int m_iLastState = -1;   // -1 = uninitialised, 0 = off, 1 = on
    };
}

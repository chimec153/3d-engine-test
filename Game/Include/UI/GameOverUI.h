#pragma once

#include "UI/UIControl.h"
#include "Core/Macro.h"
#include <memory>

namespace Engine
{
    class Button;
    class Text;
    class Font;
}

namespace Client
{
    class Player;

    // Game-over overlay. Polls the target Player each frame; when the player
    // dies (HP <= 0) it pauses the game (GameState::GameOver), draws a dark
    // backdrop + "게임 오버" caption, and shows a button that returns to the
    // start screen. Same UIControl + Button/Text composition as
    // LevelUpChoices; hidden until the player dies.
    class GAME_DLL GameOverUI : public Engine::UIControl
    {
    public:
        GameOverUI();
        virtual ~GameOverUI() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::weak_ptr<Player> m_pTarget;
        // Set when the button is pressed so Update stops re-triggering: the
        // button's ExitModal flips IsPlaying back to true while the player
        // is still dead, which would otherwise re-enter GameOver (and
        // re-stop the timer) before the deferred scene change applies.
        bool m_bDismissed = false;

        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pBtnFont;
        std::shared_ptr<Engine::Button> m_pBackdrop;
        std::shared_ptr<Engine::Text>   m_pTitleText;
        std::shared_ptr<Engine::Button> m_pButton;
        std::shared_ptr<Engine::Text>   m_pButtonText;

        void Show();
        void Hide();
        void OnButton();
    };
}

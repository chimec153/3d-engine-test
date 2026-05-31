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
    // ESC pause menu. Polls the Escape key each frame: ESC while Playing
    // pauses the game (GameState::Paused) and shows a dimmed overlay with
    // "이어하기" (resume) / "종료하기" (back to the title screen); ESC while
    // Paused resumes. Same UIControl + Button/Text composition as GameOverUI;
    // hidden until ESC opens it. Inert while a LevelUp / GameOver modal is up
    // (ESC only toggles between Playing and Paused).
    class GAME_DLL PauseMenuUI : public Engine::UIControl
    {
    public:
        PauseMenuUI();
        virtual ~PauseMenuUI() override = default;

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pBtnFont;
        std::shared_ptr<Engine::Button> m_pBackdrop;
        std::shared_ptr<Engine::Text>   m_pTitleText;
        std::shared_ptr<Engine::Button> m_pResumeBtn;
        std::shared_ptr<Engine::Text>   m_pResumeText;
        std::shared_ptr<Engine::Button> m_pQuitBtn;
        std::shared_ptr<Engine::Text>   m_pQuitText;

        void Show();
        void Hide();
        void OnResume();   // 이어하기 — resume the game
        void OnQuit();     // 종료하기 — back to the title screen
    };
}

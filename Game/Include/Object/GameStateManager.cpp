#include "GameStateManager.h"
#include "Core/Window.h"

namespace Client
{
    void GameStateManager::EnterModal(GameState eState)
    {
        if (eState == GameState::Playing) return;   // use ExitModal
        if (m_eState == eState) return;

        m_eState = eState;
        if (auto* pWin = Engine::Window::GetInst())
            if (auto pTimer = pWin->GetTimer())
                pTimer->Stop();
    }

    void GameStateManager::ExitModal()
    {
        if (m_eState == GameState::Playing) return;

        m_eState = GameState::Playing;
        if (auto* pWin = Engine::Window::GetInst())
            if (auto pTimer = pWin->GetTimer())
                pTimer->Resume();
    }
}

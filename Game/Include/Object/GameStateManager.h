#pragma once

namespace Client
{
    // Top-level game state machine. Single source of truth for "is the
    // game currently playing, or paused on a modal?". Centralising this
    // keeps the timer-stop and (future) input-routing rules in one
    // place — the old pattern had each modal toggling
    // Engine::Window::Timer itself, which broke when a modal hid
    // without cleaning up.
    //
    // Add new modal states (Inventory, GameOver, Settings, …) by
    // extending the enum and routing the corresponding UI through
    // EnterModal / ExitModal.
    enum class GameState
    {
        Playing,
        LevelUpModal,
        GameOver,
        Paused,      // ESC pause menu (이어하기 / 종료하기)
        Intermission, // between-round prep: pick the tower weapon, then start
        StartSelect,  // start-of-game free pick of one starting weapon
        // Inventory (future)
    };

    class GameStateManager
    {
    public:
        static GameStateManager& GetInst()
        {
            static GameStateManager inst;
            return inst;
        }

        GameState GetState() const { return m_eState; }
        bool      IsPlaying() const { return m_eState == GameState::Playing; }
        bool      IsPaused()  const { return m_eState != GameState::Playing; }

        // Transition into a modal state. Stops the global timer so
        // gameplay (enemy ticks, bullet movement) freezes for the
        // duration. No-op if already in the requested state.
        void EnterModal(GameState eState);

        // Pop back to Playing. Resumes the timer. No-op if already
        // Playing.
        void ExitModal();

    private:
        GameStateManager() = default;
        ~GameStateManager() = default;
        GameStateManager(const GameStateManager&)            = delete;
        GameStateManager& operator=(const GameStateManager&) = delete;

        GameState m_eState = GameState::Playing;
    };
}

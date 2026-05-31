#pragma once

#include "UI/UIControl.h"
#include "../GameDefs.h"
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

    // Level-up choice modal — three weapon cards in the centre of the
    // screen. Polls Player::HasPendingLevelUp every frame; when it
    // flips on, the card pool is recomputed (unowned weapons preferred
    // until the player's 6 slots fill, then owned weapons appear as
    // level-up cards), each card's texture is retinted to the weapon's
    // CSV colour, and the game pauses (Engine::Window::Stop). Picking
    // a card calls Player::ConsumeLevelUp(weapon_id).
    class GAME_DLL LevelUpChoices : public Engine::UIControl
    {
    public:
        LevelUpChoices();
        virtual ~LevelUpChoices() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::weak_ptr<Player> m_pTarget;
        // m_bShown removed — modal visibility is now owned by
        // GameStateManager (state == GameState::LevelUpModal).
        // Multi-level pickups drain Player::PendingLevelUpCount() by
        // re-rolling on each OnPick until the queue empties.

        // Each card has one background Button (coloured panel, owns the
        // OnClick handler) plus two Text components for the name and
        // level overlays. Text is a UIControl in its own right — it
        // owns its UIRenderer + Transform and renders itself; we
        // never bridge its texture into the Button.
        std::shared_ptr<Engine::Button> m_pBgButtons[3];
        std::shared_ptr<Engine::Font> m_pNameFont;
        std::shared_ptr<Engine::Font> m_pLvlFont;
        std::shared_ptr<Engine::Text> m_pNameTexts[3];
        std::shared_ptr<Engine::Text> m_pLvlTexts[3];

        // LevelUpDatabase catalogue index shown on each card (-1 = empty slot).
        // Kept as a plain int so this header needs no extra include.
        int m_iCardStats[3] = { -1, -1, -1 };

        void Show();
        void Hide();
        void OnPick(int iCardIndex);
        // Re-roll the 3 cards with distinct random stat upgrades; updates each
        // card's tint + label + effect text.
        void RollCards();
    };
}

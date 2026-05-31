#pragma once

#include "UI/UIControl.h"
#include "Core/Macro.h"
#include <functional>
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

    // Start-of-game weapon picker. Shown while GameStateManager is in the
    // StartSelect state (game frozen, before round 1). Lists the player's
    // crafted weapons; clicking one arms BOTH the player (a loadout slot) and
    // the towers (TowerManager) with it for free, then fires OnChosen so
    // GameScene starts round 1 and resumes. Modelled on TowerIntermissionUI
    // but with no money/prices and no separate start button — the pick is the
    // start.
    class GAME_DLL StartWeaponSelectUI : public Engine::UIControl
    {
    public:
        StartWeaponSelectUI();
        virtual ~StartWeaponSelectUI() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }
        // Called once a weapon has been chosen (GameScene: StartRound(1) +
        // ExitModal).
        void SetOnChosen(std::function<void()> fn) { m_fnChosen = std::move(fn); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        static constexpr int kMaxItems = 10;

        // Screen-pixel rect for mouse hit-testing the hover tooltip.
        struct Rect { float x = 0.f, y = 0.f, w = 0.f, h = 0.f; };
        static bool InRect(float mx, float my, const Rect& r)
        {
            return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
        }

        std::weak_ptr<Player>           m_pTarget;
        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pItemFont;
        std::shared_ptr<Engine::Text>   m_pTitle;
        std::shared_ptr<Engine::Button> m_pItemButtons[kMaxItems];
        std::shared_ptr<Engine::Text>   m_pItemTexts[kMaxItems];
        Rect                            m_ItemRect[kMaxItems];   // hover hit-test
        int                             m_iItemWeaponIds[kMaxItems];
        int                             m_iCount = 0;
        bool                            m_bShownLocal = false;
        std::function<void()>           m_fnChosen;

        // Hover tooltip — a dark panel + multi-line text showing the weapon's
        // detail stats; follows the cursor while hovering a row. Created last so
        // it draws above the rows; hidden until a hover is detected. Mirrors the
        // between-round shop (TowerIntermissionUI).
        std::shared_ptr<Engine::Button> m_pTooltipBg;
        std::shared_ptr<Engine::Text>   m_pTooltipText;

        void Show();
        void Hide();
        // Fill the rows from the player's crafted weapons (first kMaxItems).
        void BuildList();
        void OnPick(int iIndex);
        // Poll the mouse to show a weapon-detail tooltip when hovering a row.
        void HandleTooltip();
    };
}

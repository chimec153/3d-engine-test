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
    class Tower;

    // Between-round shop. Shown while GameStateManager is in the Intermission
    // state (the game is frozen). Sections:
    //   1. Buy Weapons   — a random subset of the player's crafted weapons;
    //                      clicking an unowned one pays kWeaponPrice and adds
    //                      it to the player's loadout.
    //   2. Your Weapons  — a horizontal strip of the player's owned weapons;
    //                      DRAG one onto a tower row to equip it there.
    //   3. Tower Loadout — one row per PLACED tower (drop target). A dropped
    //                      weapon sets that tower's weapon; clicking a row also
    //                      cycles it as a fallback.
    // Towers/heal-towers are no longer fixed buttons — they roll into the
    // random Buy catalog (section 1) alongside weapons.
    // Drag-and-drop is done by polling the mouse + hit-testing stored rects in
    // Update (UIControl has no drag hooks). A start button resumes play.
    class GAME_DLL TowerIntermissionUI : public Engine::UIControl
    {
    public:
        TowerIntermissionUI();
        virtual ~TowerIntermissionUI() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }
        void SetOnStartNextRound(std::function<void()> fn) { m_fnStart = std::move(fn); }
        void SetRoundNumberProvider(std::function<int()> fn) { m_fnRound = std::move(fn); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        // Buy catalog size, owned-weapon drag icons, and placed-tower rows.
        static constexpr int kBuyRows   = 4;
        static constexpr int kOwnedRows = 6;   // = player max weapon slots
        static constexpr int kTowerRows = 6;
        // Config slots for bought-but-unplaced towers (horizontal icon strip).
        // Caps the display at the typical starting stock; extra unplaced towers
        // beyond this still place FIFO using their default weapon.
        static constexpr int kReserveRows = 10;

        // Screen-pixel rect for mouse hit-testing during drag-and-drop.
        struct Rect { float x = 0.f, y = 0.f, w = 0.f, h = 0.f; };
        static bool InRect(float mx, float my, const Rect& r)
        {
            return mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h;
        }

        std::weak_ptr<Player>           m_pTarget;

        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pItemFont;

        std::shared_ptr<Engine::Text>   m_pTitle;
        std::shared_ptr<Engine::Text>   m_pInfoText;     // gold + tower count
        std::shared_ptr<Engine::Text>   m_pStatsText;    // player stat panel

        // A buy-catalog row is either a weapon or a tower type — towers now roll
        // into the same random catalog as weapons (instead of fixed buttons).
        enum class BuyKind { Weapon, Tower, HealTower };
        std::shared_ptr<Engine::Text>   m_pBuyHeader;
        std::shared_ptr<Engine::Button> m_pBuyButtons[kBuyRows];
        std::shared_ptr<Engine::Text>   m_pBuyTexts[kBuyRows];
        // Dark outline copies drawn behind each buy label (offset duplicates) so
        // coloured weapon text stays readable over the coloured button. Eight
        // directions at a 1px offset form a thin, even ring (vs a chunky blob).
        static constexpr int kOutlineCopies = 8;
        std::shared_ptr<Engine::Text>   m_pBuyTextOutline[kBuyRows][kOutlineCopies];
        BuyKind                         m_eBuyKind[kBuyRows];
        int                             m_iBuyIds[kBuyRows];   // weapon id when kind==Weapon
        int                             m_iBuyCount = 0;

        // Your Weapons — draggable owned-weapon icons (horizontal strip).
        std::shared_ptr<Engine::Text>   m_pOwnedHeader;
        std::shared_ptr<Engine::Button> m_pOwnedIcons[kOwnedRows];
        int                             m_iOwnedIds[kOwnedRows];
        Rect                            m_OwnedRect[kOwnedRows];   // hit-test

        std::shared_ptr<Engine::Text>   m_pTowerHeader;
        std::shared_ptr<Engine::Button> m_pTowerButtons[kTowerRows];
        std::shared_ptr<Engine::Text>   m_pTowerTexts[kTowerRows];
        // The placed tower shown on each loadout row (rebuilt each refresh;
        // stable during a shop session since the game is frozen).
        std::weak_ptr<Tower>            m_pTowerRowRefs[kTowerRows];
        Rect                            m_TowerRect[kTowerRows];   // drop target
        int                             m_iTowerCount = 0;

        // Unplaced Towers — per-reserve-tower weapon config (drag target +
        // click to cycle). Icons only (weapon shown by colour, like the owned
        // strip); index 0 = the next tower placed (FIFO).
        std::shared_ptr<Engine::Text>   m_pReserveHeader;
        std::shared_ptr<Engine::Button> m_pReserveIcons[kReserveRows];
        Rect                            m_ReserveRect[kReserveRows];
        int                             m_iReserveCount = 0;

        std::shared_ptr<Engine::Button> m_pStartButton;
        std::shared_ptr<Engine::Text>   m_pStartText;

        // Drag-and-drop state: the weapon being dragged + a ghost icon that
        // follows the cursor while dragging.
        bool m_bDragging       = false;
        int  m_iDragWeaponId   = -1;
        std::shared_ptr<Engine::Button> m_pDragGhost;

        bool m_bShownLocal = false;
        std::function<void()> m_fnStart;
        std::function<int()>  m_fnRound;

        void Show();
        void Hide();
        // Roll a fresh random buy catalog (once per shop open).
        void RollCatalog();
        // Replace a single bought slot with a fresh catalog pick (different from
        // the items currently shown in the other slots).
        void RerollBuySlot(int iIndex);
        // Refresh every label/price/highlight from current state.
        void RebuildList();
        // Live "Tower" objects in the scene (placed count, for the HUD line).
        int  PlacedTowerCount() const;

        // Buy the i-th catalog row — a weapon or a tower, per m_eBuyKind[i].
        void OnBuyItem(int iIndex);
        // Cycle the i-th placed tower's weapon to the player's next owned one.
        void OnCycleTowerWeapon(int iIndex);
        // Cycle the i-th unplaced (reserve) tower's weapon to the next owned one.
        void OnCycleReserveWeapon(int iIndex);
        void OnStart();
        // Poll the mouse to run weapon→tower drag-and-drop (called from Update
        // while the shop is open).
        void HandleDrag();
    };
}

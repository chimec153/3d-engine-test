#pragma once

#include "UI/UIControl.h"
#include "Core/Macro.h"
#include <functional>
#include <memory>
#include <vector>

namespace Engine
{
    class Font;
    class Text;
    class Button;
    class GameObject;
}

namespace Client
{
    class Player;
    class Tower;
    class HealTower;

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
        Rect                            m_BuyRect[kBuyRows];   // hit-test (hover tooltip)
        // Per-slot "has an item" flag (replaces a contiguous count: a pinned
        // slot can sit anywhere, so the used set isn't a prefix any more).
        bool                            m_bBuyUsed[kBuyRows];
        // Pin/lock per slot — a pinned slot keeps its item across shop opens and
        // post-purchase rerolls (RollCatalog / RerollBuySlot skip it). Persists
        // for the session (this component is created once and reused each round).
        bool                            m_bBuyLocked[kBuyRows];
        std::shared_ptr<Engine::Button> m_pLockButtons[kBuyRows];
        std::shared_ptr<Engine::Text>   m_pLockTexts[kBuyRows];

        // Equipped weapons — the player's firing slots (≤ Player kMaxEquipSlots).
        // L-click an icon to UNEQUIP it (→ inventory); R-click for the menu
        // (sell / merge / equip-to-tower). Empty firing slots show a dim [+].
        std::shared_ptr<Engine::Text>   m_pOwnedHeader;
        std::shared_ptr<Engine::Button> m_pOwnedIcons[kOwnedRows];
        std::shared_ptr<Engine::Text>   m_pOwnedLvlTexts[kOwnedRows];   // per-copy "Lv.N"
        int                             m_iOwnedIds[kOwnedRows];
        Rect                            m_OwnedRect[kOwnedRows];   // hit-test

        // Weapon inventory — owned weapons that are neither equipped nor on a
        // tower (idle). L-click to EQUIP (→ a free firing slot); R-click for the
        // menu (sell / merge / equip-to-tower).
        static constexpr int kInvRows = 6;
        std::shared_ptr<Engine::Text>   m_pInvHeader;
        std::shared_ptr<Engine::Button> m_pInvIcons[kInvRows];
        std::shared_ptr<Engine::Text>   m_pInvLvlTexts[kInvRows];   // per-copy "Lv.N"
        int                             m_iInvIds[kInvRows] = {};
        Rect                            m_InvRect[kInvRows];

        std::shared_ptr<Engine::Text>   m_pTowerHeader;
        std::shared_ptr<Engine::Button> m_pTowerButtons[kTowerRows];
        std::shared_ptr<Engine::Text>   m_pTowerTexts[kTowerRows];
        // The placed tower shown on each loadout row (rebuilt each refresh;
        // stable during a shop session since the game is frozen). Holds either
        // an attack Tower or a HealTower as the common GameObject base;
        // m_bTowerRowIsHeal[i] says which (heal rows have no weapon to cycle).
        std::weak_ptr<Engine::GameObject> m_pTowerRowRefs[kTowerRows];
        bool                            m_bTowerRowIsHeal[kTowerRows] = {};
        Rect                            m_TowerRect[kTowerRows];   // drop target
        int                             m_iTowerCount = 0;
        // Placed and unplaced towers now share ONE list (acquisition order); each
        // row is either a placed scene object (m_pTowerRowRefs) or an index into
        // TowerManager's attack/heal reserve. m_eTowerRowSrc says which.
        enum class TowerRowSrc { Placed, AtkReserve, HealReserve };
        TowerRowSrc                     m_eTowerRowSrc[kTowerRows] = {};
        int                             m_iTowerRowReserve[kTowerRows] = {};   // reserve index for reserve rows
        int                             m_iTowerRowDefId[kTowerRows] = {};     // towers.csv type (merge grouping)
        int                             m_iTowerRowLevel[kTowerRows] = {};

        // Unplaced Towers — per-reserve-tower weapon config (drag target +
        // click to cycle). Icons only (weapon shown by colour, like the owned
        // strip); index 0 = the next tower placed (FIFO).
        std::shared_ptr<Engine::Text>   m_pReserveHeader;
        std::shared_ptr<Engine::Button> m_pReserveIcons[kReserveRows];
        Rect                            m_ReserveRect[kReserveRows];
        int                             m_iReserveCount = 0;

        std::shared_ptr<Engine::Button> m_pStartButton;
        std::shared_ptr<Engine::Text>   m_pStartText;

        // Reroll — a button under the buy rows that re-rolls every unpinned buy
        // slot for a round-scaled fee (kReroll* in GameDefs). Pinned slots are
        // kept (RollCatalog skips them).
        std::shared_ptr<Engine::Button> m_pRerollButton;
        std::shared_ptr<Engine::Text>   m_pRerollText;

        // Weapon action menu — clicking an owned-weapon icon pops a small panel
        // with Sell / Merge / Equip (replaces the old drag-to-equip +
        // right-click-to-sell). m_iMenuWeaponId >= 0 means the menu is open and
        // is the weapon it acts on. Created last so it draws above the rows.
        static constexpr int kMenuRows = 3;   // 0 Sell, 1 Merge, 2 Equip
        std::shared_ptr<Engine::Button> m_pMenuBg;
        std::shared_ptr<Engine::Button> m_pMenuButtons[kMenuRows];
        std::shared_ptr<Engine::Text>   m_pMenuTexts[kMenuRows];
        int m_iMenuWeaponId = -1;
        // The same panel doubles as a TOWER action menu (Merge / Weapon / Sell)
        // when a placed-tower row is clicked. >= 0 = open for that tower row;
        // mutually exclusive with m_iMenuWeaponId. The shared menu buttons route
        // to the tower handlers when this is set (see Init's OnClick lambdas).
        int m_iMenuTowerRow = -1;
        Rect m_MenuPanelRect;   // bg-panel rect for click-outside dismissal

        // "Equip to tower" arm state: after clicking Equip, the next click on a
        // tower-loadout row or reserve slot assigns this weapon there (the menu
        // replaces the old drag). -1 = not armed; a ghost follows the cursor
        // while armed. m_pDragGhost is reused as that ghost.
        int m_iEquipArmedWeaponId = -1;
        // Set the frame Equip is clicked so HandleWeaponMenu doesn't treat that
        // same click as an off-target cancel; cleared on the next poll.
        bool m_bEquipArmedThisFrame = false;
        std::shared_ptr<Engine::Button> m_pDragGhost;

        // Double-click detection for the equip/inventory icons. Intermission dt
        // is ~0 (the world is frozen), so the click gap is measured in FRAMES
        // (Update still ticks). m_iClickFrame advances each Update.
        int m_iClickFrame     = 0;
        int m_iLastClickFrame = -1000;
        int m_iLastClickKey   = -1;   // last icon clicked (encoded: equip/inv + index)
        static constexpr int kDoubleClickFrames = 18;   // ~0.3s at 60fps

        // Real drag-and-drop: press on an equipped / inventory weapon icon to
        // pick it up (the ghost follows the cursor), release over a target to
        // move it — inventory→equip, equipped→inventory, either→a tower slot.
        // -1 = not dragging. Distinct from the menu's click-to-arm equip flow.
        enum class DragSrc { None, Equipped, Inventory };
        int     m_iDragWeaponId = -1;
        DragSrc m_eDragSrc      = DragSrc::None;
        // Poll the mouse to drive a press-drag-release weapon move (Update).
        void HandleDrag();

        // Hover tooltip — a dark panel + multi-line text showing the weapon's
        // detail stats; follows the cursor while hovering a weapon (buy row or
        // owned icon). Created last so it draws above the rest; hidden until a
        // hover is detected in HandleTooltip.
        std::shared_ptr<Engine::Button> m_pTooltipBg;
        std::shared_ptr<Engine::Text>   m_pTooltipText;

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
        // True when a weapon id is mounted on any tower (placed or reserve).
        // Such weapons are locked to towers, so the buy catalog hides them — a
        // weapon is owned by EITHER the player OR a tower, never both.
        bool IsWeaponHeldByTower(int iWeaponId) const;
        // Shop price for a weapon id — the WeaponDef's per-weapon iPrice, or the
        // global kWeaponPrice default when that's 0/unset. Used by buy / merge /
        // sell-refund so a weapon can be priced individually.
        int  WeaponPriceOf(int iWeaponId) const;

        // Buy the i-th catalog row — a weapon or a tower, per m_eBuyKind[i].
        void OnBuyItem(int iIndex);
        // Toggle the i-th slot's pin (keeps that item through rerolls).
        void OnToggleLock(int iIndex);
        // Sell an owned weapon (Sell in the weapon menu): refund gold and drop
        // it from the player's loadout.
        void OnSellWeapon(int iWeaponId);
        // Cycle the i-th placed tower's weapon to the player's next owned one.
        // When a weapon is armed (Equip in the menu), the click equips that
        // weapon to this tower instead of cycling.
        void OnCycleTowerWeapon(int iIndex);
        // Sell the i-th placed tower (right-click its loadout row): remove it
        // from the scene, free the owned-tower slot, and refund half kTowerPrice.
        void OnSellTower(int iIndex);
        // Cycle the i-th unplaced (reserve) tower's weapon to the next owned one
        // (or equip an armed weapon, mirroring OnCycleTowerWeapon).
        void OnCycleReserveWeapon(int iIndex);
        void OnStart();
        // Re-roll all unpinned buy slots, charging RerollCost (no-op if every
        // slot is pinned or the player can't afford it).
        void OnReroll();
        // Reroll fee for the upcoming round: kRerollBaseCost grows linearly by
        // kRerollCostPerRound each round.
        int  RerollCost() const;

        // Weapon action menu. OpenWeaponMenu pops the panel for the owned-icon
        // at iOwnedIndex; the three buttons run the actions; CloseWeaponMenu
        // hides it. Merge = combine two owned copies of the same weapon into
        // one higher-level copy (free); Equip arms the weapon for a
        // tower/reserve click.
        void OpenWeaponMenu(int iWeaponId, const Rect& anchor);
        void CloseWeaponMenu();
        void OnMenuSell();
        void OnMenuMerge();
        void OnMenuEquip();
        // Equipped-strip icon → unequip that weapon (→ inventory).
        void OnEquipSlotClick(int iIndex);
        // Inventory-strip icon → equip that weapon (→ a free firing slot).
        void OnInventoryClick(int iIndex);
        // Double-click router for the equipped (bEquipped=true) / inventory
        // weapon icons: a SINGLE click does nothing; only a quick second click
        // toggles equip/unequip (a single click was too easy to trigger by
        // accident). bEquipped picks which handler to run on the double-click.
        void OnWeaponIconClick(bool bEquipped, int iIndex);

        // Tower action menu — the placed-tower analogue of the weapon menu,
        // popped from a tower-loadout row click (reuses the same panel/buttons).
        // OpenTowerMenu lays it out under row iRow; the three actions mirror the
        // weapon menu: Merge combines two placed towers that share this tower's
        // weapon into one a level higher (free, the cost is the consumed tower);
        // Weapon cycles the equipped weapon; Sell removes + refunds.
        void OpenTowerMenu(int iRow);
        void OnTowerMenuMerge();
        void OnTowerMenuCycle();
        void OnTowerMenuSell();
        // Unified tower-row click: assign an armed weapon (placed or reserve) or
        // open the action menu. Right-click sells. Dispatch by m_eTowerRowSrc.
        void OnTowerRowClick(int iRow);
        void OnTowerRowSell(int iRow);
        // Resolve a stored tower-type id to its canonical id: a default attack
        // tower stores -1 in the reserve but the resolved FirstOfKind(Attack) id
        // once placed, so merge must normalise both to group them as one TYPE.
        int  ResolveTowerType(int iTowerDefId) const;
        // Count owned towers (placed + unplaced) of a resolved TYPE, and the
        // highest level among them — backs the merge enable + its "(xN)" label.
        void CountTowersOfType(int iResolvedType, int& outCount, int& outMaxLevel) const;
        // Same for HEAL towers (all one type): owned heal count (placed +
        // unplaced) and the highest heal-tower level.
        void CountHealTowers(int& outCount, int& outMaxLevel) const;
        // True when the action menu is open and the cursor is over its panel —
        // the row buttons underneath bail on their click so it goes to the menu
        // (each UIControl hit-tests independently, so overlaps would double-fire).
        bool PointerInOpenMenu() const;
        // Poll for menu dismissal (click outside) + drive the equip-arm ghost
        // (called from Update while the shop is open).
        void HandleWeaponMenu();
        // Poll the mouse to show a weapon-detail tooltip when hovering a buy row
        // or owned icon (called from Update while the shop is open).
        void HandleTooltip();
    };
}

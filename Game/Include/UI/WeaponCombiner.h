#pragma once

#include "UI/UIControl.h"
#include <memory>
#include <string>
#include <vector>

namespace Engine
{
    class Button;
    class Text;
    class Font;
}

namespace Client
{
    // One craftable weapon part ("부품 카드"), loaded from parts.csv (with a
    // built-in fallback). iCategory is the slot/category index (0..5);
    // iVariant is the underlying WeaponData enum value for that category.
    // fAmount is the per-level bump for the LevelUp category (ignored by
    // the others); fDuration is the weapon lifetime in seconds for the
    // FireMode category (so two parts of the same fire mode can differ;
    // ignored by the others). uColor tints the icon, the equipped slot,
    // and — for a LevelUp part — the crafted weapon.
    struct PartCard
    {
        int          iCategory = 0;
        int          iVariant  = 0;
        std::wstring wLabel;
        unsigned int uColor    = 0xFFFFFF;
        float        fAmount   = 0.f;
        float        fDuration = 0.f;
    };

    // Weapon-combination screen body (lives inside WeaponComboScene).
    //
    // Six type-fixed equip slots — one per weapon-property category
    // (SpawnOrigin / MovementType / FireMode / OnHitEvent /
    // ProjectileShape / Element). Below them, an "inventory" of square
    // attribute icons, one row per category. Double-clicking an icon
    // equips it into its category's slot (replacing any prior choice).
    //
    // When all six slots are filled the craft button lights up; clicking
    // it assembles a real WeaponDef from the chosen attributes and adds
    // it to WeaponDatabase (so it appears in the level-up pool in game).
    //
    // Same UIControl + child Button/Text composition as LevelUpChoices /
    // StartMenu. Components are created in Init and live for the scene.
    class WeaponCombiner : public Engine::UIControl
    {
    public:
        WeaponCombiner();
        WeaponCombiner(const WeaponCombiner& other) = default;
        virtual ~WeaponCombiner() override = default;

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        static constexpr int kCatCount = 6;
        // Registry display capacity (right-hand loadout panel). The equip
        // loadout cap (10) lives in WeaponDatabase::kMaxEquipped; this is
        // a couple larger so you can craft spares and swap which ten are
        // equipped.
        static constexpr int kRegCells = 12;

        // One inventory click. Detects a double-click (same icon twice
        // within kDoubleClickSec) and equips on the second hit.
        void OnIconClick(int iPaletteIndex);
        // Sets the icon's category slot to this attribute and refreshes
        // the slot panel + craft button.
        void Equip(int iPaletteIndex);
        // Greys/greens the craft button and updates m_bCraftReady.
        void RefreshCraft();
        // Assembles the WeaponDef from the six slots and registers it.
        void OnCraft();
        // Single click on a registry cell — toggles that crafted weapon's
        // equip state (respecting the loadout cap) and refreshes.
        void OnRegistryClick(int iCell);
        // Repaints the registry cells + header from WeaponDatabase's
        // crafted list and equip flags.
        void RefreshRegistry();

        // Slot widgets, indexed by category.
        std::shared_ptr<Engine::Button> m_pSlotButton[kCatCount];
        std::shared_ptr<Engine::Text>   m_pSlotNameText[kCatCount];
        std::shared_ptr<Engine::Text>   m_pSlotCatText[kCatCount];
        // Palette index currently equipped per slot, -1 = empty.
        int                             m_iSlot[kCatCount] = { -1, -1, -1, -1, -1, -1 };

        // Part-card catalogue loaded from parts.csv (or built-in defaults).
        // m_iSlot and the inventory icons index into this.
        std::vector<PartCard> m_parts;

        // Inventory icons (parallel to the palette order).
        std::vector<std::shared_ptr<Engine::Button>> m_iconButtons;
        std::vector<std::shared_ptr<Engine::Text>>   m_iconTexts;

        std::shared_ptr<Engine::Button> m_pCraftButton;
        std::shared_ptr<Engine::Text>   m_pCraftText;
        std::shared_ptr<Engine::Text>   m_pResultText;
        std::shared_ptr<Engine::Text>   m_pTitleText;
        std::shared_ptr<Engine::Text>   m_pInvHeader;
        std::shared_ptr<Engine::Button> m_pBackButton;
        std::shared_ptr<Engine::Text>   m_pBackText;

        // Right-hand "crafted weapons" loadout panel.
        std::shared_ptr<Engine::Text>   m_pRegHeader;
        std::shared_ptr<Engine::Button> m_pRegButton[kRegCells];
        std::shared_ptr<Engine::Text>   m_pRegText[kRegCells];

        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pMidFont;
        std::shared_ptr<Engine::Font>   m_pSmallFont;

        bool  m_bCraftReady       = false;
        int   m_iCraftCounter     = 0;

        // Double-click tracking. m_fTime is monotonic scene time.
        float m_fTime             = 0.f;
        int   m_iLastClickPalette = -1;
        float m_fLastClickTime    = -10.f;
    };
}

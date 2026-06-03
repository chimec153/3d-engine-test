#pragma once

#include "UI/UIControl.h"
#include <memory>
#include <string>
#include <vector>
#include <deque>

namespace Engine
{
    class Button;
    class Text;
    class Font;
    class ScrollView;
    class NumberField;
    class EditBox;
    class Transform;
}

namespace Client
{
    struct WeaponDef;   // defined in ../Object/WeaponData.h (used by AssembleWeaponDef)
    class Bullet;       // live-preview projectiles fired from the editor state

    // One craftable weapon part ("부품 카드"), loaded from parts.csv (with a
    // built-in fallback). iCategory is the slot/category index (0..5);
    // iVariant is the underlying WeaponData enum value for that category.
    // fAmount is a per-category number: the per-level bump for a LevelUp
    // part, or the uniform scale for a Size part (ignored by the others).
    // fDuration is the weapon lifetime in seconds for a FireMode part (so
    // two parts of the same fire mode can differ; ignored by the others).
    // fGrowth is the orbital radial growth (world units/sec) for a Movement
    // part — >0 makes an Orbital weapon spiral outward; ignored otherwise.
    // fDamageInterval is the Field on-hit DoT tick interval (seconds); read
    // only by the Field on-hit card, ignored by every other category.
    // uColor tints the icon, the equipped slot, and — for a LevelUp part —
    // the crafted weapon.
    struct PartCard
    {
        int          iCategory = 0;
        int          iVariant  = 0;
        std::wstring wLabel;
        unsigned int uColor    = 0xFFFFFF;
        float        fAmount   = 0.f;
        float        fDuration = 0.f;
        float        fGrowth   = 0.f;
        float        fDamageInterval = 0.f;
        // Movement (Homing / Aimed) only: AimMode index (0 Nearest / 1 LowestHP
        // / 2 Random). Ignored by other categories; 0 = Nearest default.
        float        fAimMode  = 0.f;
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
        static constexpr int kCatCount = 9;
        // Weapon-list capacity (right-hand panel). Sized to hold the whole
        // weapons_v2 catalogue (plus room for weapons added via the "add weapon"
        // button) so every weapon is selectable for editing; weapons beyond this
        // are not shown (logged-free cap). The list scrolls, so a high cap is fine.
        static constexpr int kRegCells = 64;

        // One inventory click. Detects a double-click (same icon twice
        // within kDoubleClickSec) and equips on the second hit.
        void OnIconClick(int iPaletteIndex);
        // Sets the icon's category slot to this attribute and refreshes
        // the slot panel + craft button.
        void Equip(int iPaletteIndex);
        // Greys/greens the craft button and updates m_bCraftReady.
        void RefreshCraft();
        // Repaints the impact category after a toggle: dims/brightens each
        // impact icon by equipped state and updates the impact slot display.
        void RefreshImpactSlot();
        // Same as RefreshImpactSlot but for the multi-select level-up category:
        // dims/brightens each level-up icon by selection + shows the count.
        void RefreshLevelUpSlot();
        // Builds a WeaponDef from the current editor state (slots + number
        // fields). Valid only when all card slots are filled — used by OnSave
        // and by the live power-score preview.
        WeaponDef AssembleWeaponDef() const;
        // Save the current editor state back onto the selected weapons_v2
        // weapon (m_iEditId), preserving fields the editor doesn't control
        // (id / name / colour / price / evolution / shop_available / trail),
        // then persist the whole catalogue to weapons_v2.csv.
        void OnSave();
        // Add the current editor build as a BRAND-NEW weapon in the catalogue
        // (fresh id) instead of overwriting the selected one, persist to
        // weapons_v2.csv, then select the new weapon for further editing.
        // Requires all card slots filled (same gate as OnSave).
        void OnAddWeapon();
        // Delete the weapon currently being edited from the catalogue, persist
        // to weapons_v2.csv, then clear the editor. No-op if nothing is selected.
        void OnDeleteWeapon();
        // Load an existing weapons_v2 weapon's attributes into the editor for
        // editing (reverse-maps the def onto the card slots + number fields).
        void LoadWeaponIntoEditor(int iWeaponId);
        // Deselect the edited weapon: clear m_iEditId and reset every slot /
        // number field / toggle to its blank-build default. Invoked by clicking
        // the already-edited registry cell again (a deselect toggle).
        void ClearEditor();
        // Toggles Sustained (persistent) fire mode: greys the cooldown +
        // lifetime number fields and recolours the toggle button.
        void OnSustainToggle();
        // Flips shop availability + recolours the toggle (the shop-side analogue
        // of OnSustainToggle).
        void OnShopToggle();
        // Single click on a registry cell — selects that weapons_v2 weapon and
        // loads it into the editor.
        void OnRegistryClick(int iCell);
        // Repaints the registry cells + header from the weapons_v2 catalogue,
        // highlighting the one currently being edited.
        void RefreshRegistry();

        // --- Live firing preview ---
        // Drives the in-scene weapon preview each frame: re-frames the camera on
        // the fixed muzzle lane and auto-fires the currently-assembled WeaponDef
        // forward on its own cooldown (sustained weapons persist; any edit
        // restarts the preview). Only fires while the build is craft-ready.
        void UpdatePreview(float fDeltaTime);
        // Spawns one cooldown burst of preview bullets from the muzzle, reusing
        // the same fan/spawn geometry as Player::FireCooldownBurst (minus the
        // enemies / crit / voxel-reflect that need a live gameplay scene).
        void FirePreviewBurst(const WeaponDef& def);
        // InActivates every live preview bullet (on edit / when not craft-ready).
        void ClearPreview();
        // Cheap hash of the appearance-affecting WeaponDef fields, so an edit is
        // detected and the preview restarts cleanly.
        unsigned long long PreviewSignature(const WeaponDef& def) const;

        // Orbital owner anchor (a bare Transform at the muzzle) — Orbital
        // movement circles this. Created lazily on the first burst.
        std::weak_ptr<Engine::Transform>  m_pPreviewAnchor;
        // Live preview projectiles, oldest first; weak so natural despawn /
        // ClearPreview lets the Layer drop them. Used to bound the count and to
        // know when a sustained build needs (re)spawning.
        std::deque<std::weak_ptr<Bullet>> m_previewBullets;
        // Cooldown accumulator for the non-sustained preview firing loop.
        float                             m_fPreviewFireAcc = 0.f;
        // Signature of the build last previewed; a change restarts the preview.
        unsigned long long                m_uPreviewSig     = 0ull;

        // Slot widgets, indexed by category.
        std::shared_ptr<Engine::Button> m_pSlotButton[kCatCount];
        std::shared_ptr<Engine::Text>   m_pSlotNameText[kCatCount];
        std::shared_ptr<Engine::Text>   m_pSlotCatText[kCatCount];
        // Palette index currently equipped per slot, -1 = empty. CAT_IMPACT
        // is multi-select (see m_impactSel) so its entry here is unused.
        int                             m_iSlot[kCatCount] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };

        // Impact category is multi-select: a weapon can stack several modules
        // (Damage baseline + Knockback + Gather). Holds the palette indices of
        // the equipped impact cards instead of using the single m_iSlot entry.
        std::vector<int>                m_impactSel;
        // Level-up category is multi-select too: a weapon can grow several
        // stats per level. Palette indices of the chosen level-up cards (its
        // m_iSlot entry is unused, like CAT_IMPACT).
        std::vector<int>                m_levelUpSel;

        // Part-card catalogue loaded from parts.csv (or built-in defaults).
        // m_iSlot and the inventory icons index into this.
        std::vector<PartCard> m_parts;

        // Inventory icons (parallel to the palette order).
        std::vector<std::shared_ptr<Engine::Button>> m_iconButtons;
        std::vector<std::shared_ptr<Engine::Text>>   m_iconTexts;

        // Single vertical scroll for the whole left attribute panel: card
        // categories show their cards as a wrapped grid, numeric categories
        // (and the three core-stat fields) show a NumberField, and everything
        // is registered with this one view so the column scrolls as a unit.
        std::shared_ptr<Engine::ScrollView> m_pInvScroll;

        // Numeric inputs replacing the LIFETIME / FIRERATE / SIZE / ACCEL card
        // rows: keyboard-typeable + slider fields (Engine::NumberField). The
        // FIRERATE field holds the cooldown in seconds; a separate toggle picks
        // Sustained (persistent) mode, which greys the cooldown + lifetime
        // fields. AssembleWeaponDef reads these directly instead of m_iSlot.
        std::shared_ptr<Engine::NumberField> m_pNumLifetime;
        std::shared_ptr<Engine::NumberField> m_pNumCooldown;
        std::shared_ptr<Engine::NumberField> m_pNumSize;
        std::shared_ptr<Engine::NumberField> m_pNumAccel;
        // Orbital path radius (world/cell units; only used when the movement is
        // Orbital). Editable so the orbit size can be tuned and previewed live;
        // AssembleWeaponDef reads it into def.fOrbitRadius. Equip seeds it from
        // the chosen movement card's preset (Orbital 0.9 / Follow 0).
        std::shared_ptr<Engine::NumberField> m_pNumOrbitRadius;
        // Random-origin ring radius (world units; only used when the spawn
        // origin is Random). AssembleWeaponDef reads it into def.fSpawnRadius.
        std::shared_ptr<Engine::NumberField> m_pNumSpawnRadius;
        // Core stat fields (top strip) — editable now that the combiner is a
        // full weapon editor. AssembleWeaponDef reads these for iDamage /
        // iCount / fProjectileSpeed (previously hard-coded).
        std::shared_ptr<Engine::NumberField> m_pNumDamage;
        std::shared_ptr<Engine::NumberField> m_pNumCount;
        std::shared_ptr<Engine::NumberField> m_pNumSpeed;
        // Shop appearance window + multi-shot spread, previously preserved from
        // the base weapon but not editable. AssembleWeaponDef reads these into
        // def.iMinRound / def.iMaxRound / def.fSpreadDeg. min/max round 0 = no
        // bound; spread 0 = single shot / no fan.
        std::shared_ptr<Engine::NumberField> m_pNumMinRound;
        std::shared_ptr<Engine::NumberField> m_pNumMaxRound;
        std::shared_ptr<Engine::NumberField> m_pNumSpread;
        // Shop price (0 = global kWeaponPrice default) + evolution chain (target
        // weapon id, 0 = never evolves; and the level it evolves at). Read by
        // AssembleWeaponDef into def.iPrice / iEvolvesInto / iEvolveMinLevel.
        std::shared_ptr<Engine::NumberField> m_pNumPrice;
        std::shared_ptr<Engine::NumberField> m_pNumEvolveInto;
        std::shared_ptr<Engine::NumberField> m_pNumEvolveLevel;
        std::shared_ptr<Engine::Button>      m_pSustainBtn;
        std::shared_ptr<Engine::Text>        m_pSustainText;
        bool                                 m_bSustained = false;
        // Shop-availability toggle (on = the weapon can roll in the between-round
        // shop / start picker). Mirrors the Sustained toggle; read into
        // def.bShopAvailable. Defaults on, like a fresh WeaponDef.
        std::shared_ptr<Engine::Button>      m_pShopBtn;
        std::shared_ptr<Engine::Text>        m_pShopText;
        bool                                 m_bShopAvailable = true;

        // Editable weapon name (text-mode EditBox in the top bar). Read at save
        // time; populated from the selected weapon in LoadWeaponIntoEditor.
        std::shared_ptr<Engine::EditBox>     m_pNameBox;

        std::shared_ptr<Engine::Button> m_pCraftButton;
        std::shared_ptr<Engine::Text>   m_pCraftText;
        // "Add weapon" button beside Save — creates a new catalogue entry from
        // the current build instead of overwriting the selected weapon. Disabled
        // (greyed) while a weapon is being edited (m_iEditId >= 0).
        std::shared_ptr<Engine::Button> m_pAddButton;
        std::shared_ptr<Engine::Text>   m_pAddText;
        // "Delete weapon" button — removes the weapon currently being edited from
        // the catalogue. Enabled only while a weapon is selected (m_iEditId >= 0).
        std::shared_ptr<Engine::Button> m_pDeleteButton;
        std::shared_ptr<Engine::Text>   m_pDeleteText;
        std::shared_ptr<Engine::Text>   m_pScoreText;   // live power-score preview
        std::shared_ptr<Engine::Text>   m_pResultText;
        std::shared_ptr<Engine::Text>   m_pTitleText;
        std::shared_ptr<Engine::Text>   m_pInvHeader;
        std::shared_ptr<Engine::Button> m_pBackButton;
        std::shared_ptr<Engine::Text>   m_pBackText;

        // Right-hand weapon-list panel: every weapons_v2 weapon, click to edit.
        // The list scrolls vertically (the catalogue can exceed the panel
        // height); RefreshRegistry lays cells in content space + registers them
        // with the scroll view, which clips/scrolls them to its viewport.
        std::shared_ptr<Engine::Text>       m_pRegHeader;
        std::shared_ptr<Engine::Button>     m_pRegButton[kRegCells];
        std::shared_ptr<Engine::Text>       m_pRegText[kRegCells];
        std::shared_ptr<Engine::ScrollView> m_pRegScroll;
        // Cached cell geometry (set in Init, used by RefreshRegistry).
        float m_fRegX0 = 0.f, m_fRegTopY = 0.f, m_fRegCellW = 0.f;
        float m_fRegCellH = 0.f, m_fRegRowH = 0.f, m_fRegGapX = 0.f;

        std::shared_ptr<Engine::Font>   m_pTitleFont;
        std::shared_ptr<Engine::Font>   m_pMidFont;
        std::shared_ptr<Engine::Font>   m_pSmallFont;

        bool  m_bCraftReady       = false;

        // The weapons_v2 weapon currently loaded for editing (its id). -1 = none
        // selected; OnSave refuses until a weapon is picked from the list.
        int   m_iEditId           = -1;

        // Double-click tracking. m_fTime is monotonic scene time.
        float m_fTime             = 0.f;
        int   m_iLastClickPalette = -1;
        float m_fLastClickTime    = -10.f;
    };
}

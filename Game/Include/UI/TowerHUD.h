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
    class TowerPlacementController;

    // Top-RIGHT corner HUD (the WeaponHUD owns the top-left): one square box per
    // owned tower (attack AND heal), in acquisition (purchase) order — the same
    // ordered list the placement controller maps the number keys onto, so the
    // number caption BELOW each icon IS the key that deploys it. Attack boxes are TYPE-
    // coloured (towers.csv def id, palette in TowerHUD.cpp) with the equipped
    // WEAPON's colour in the bottom-left and a "Lv.N" badge bottom-right; heal
    // boxes are green with no weapon/level. A status badge along the TOP reads
    // PLACED (on the field), READY (waiting to deploy), NO WPN (weaponless attack
    // tower, can't deploy) or CD (destroyed this round, benched until next). The
    // list is built each frame by BuildTowerSlots (placed scene objects + the
    // unplaced attack/heal reserves), capped at kMaxTowers.
    //
    // Pressing a slot's number key — or clicking a READY box — enters placement
    // for THAT specific tower; PLACED / CD / weaponless boxes do nothing.
    class GAME_DLL TowerHUD : public Engine::UIControl
    {
    public:
        TowerHUD();
        virtual ~TowerHUD() override = default;

        // Clicking a READY slot box enters attack-tower placement for that
        // specific unplaced tower (the controller deploys the chosen reserve).
        void SetTarget(const std::weak_ptr<TowerPlacementController>& pPlacement)
        {
            m_pPlacement = pPlacement;
        }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        // Attack towers can never exceed the combined tower cap, so it doubles
        // as the row width.
        static constexpr int kSlotCount = kMaxTowers;

        std::weak_ptr<TowerPlacementController> m_pPlacement;

        std::shared_ptr<Engine::Font> m_pNameFont;
        std::shared_ptr<Engine::Font> m_pLvlFont;

        std::shared_ptr<Engine::Button> m_pBoxes[kSlotCount];       // type-coloured background
        std::shared_ptr<Engine::Button> m_pWeaponDots[kSlotCount];  // bottom-left weapon-colour square
        std::shared_ptr<Engine::Text>   m_pNumTexts[kSlotCount];    // top-left "1".."5" key number
        std::shared_ptr<Engine::Text>   m_pNameTexts[kSlotCount];   // tower type name (centred)
        std::shared_ptr<Engine::Text>   m_pStatusTexts[kSlotCount]; // top badge: PLACED / READY / CD
        std::shared_ptr<Engine::Text>   m_pLvlTexts[kSlotCount];    // bottom-right tower "Lv.N"
        std::shared_ptr<Engine::Text>   m_pWpnLvlTexts[kSlotCount]; // weapon level, on the weapon dot

        // Hover tooltip: dark panel + text following the cursor — the box shows
        // tower stats, the inner weapon dot shows the equipped weapon's stats.
        // Slot box + dot rects are cached at Init for the per-frame hit-test.
        std::shared_ptr<Engine::Button> m_pTipBg;
        std::shared_ptr<Engine::Text>   m_pTipText;
        float m_fBoxX[kSlotCount] = {};
        float m_fBoxY = 0.f, m_fBoxSize = 0.f;
        float m_fDotX[kSlotCount] = {};
        float m_fDotY = 0.f, m_fDotSize = 0.f;
        // Current per-slot weapon level (for the dot badge + tooltip), tracked
        // for change-detection like the other m_iLast* fields.
        int  m_iLastWpnLvl[kSlotCount];

        // Per-slot click->deploy mapping this frame (a box click / number key
        // deploys the SPECIFIC tower the slot shows). m_iReserveIdx = attack
        // reserve index, or -1; m_bHealDeploy = this slot is a ready heal tower.
        // Both are -1/false for non-deployable slots (placed / cooldown /
        // weaponless / empty) → clicking does nothing.
        int  m_iReserveIdx[kSlotCount];
        bool m_bHealDeploy[kSlotCount];

        // Change-detection: only re-bind the texture / re-rasterise the text on
        // slots whose tower type, weapon id, level OR state changed this frame.
        // State: -1 empty, 0 placed, 1 ready, 2 on destroy-cooldown.
        int m_iLastTowerIds[kSlotCount];
        int m_iLastIds[kSlotCount];
        int m_iLastLevels[kSlotCount];
        int m_iLastStates[kSlotCount];
        int m_iLastHeal[kSlotCount];   // 1 = heal slot, 0 = attack (change-detect)
    };
}

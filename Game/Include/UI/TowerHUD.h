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
    // Top-RIGHT corner HUD (the WeaponHUD owns the top-left): one square box
    // per owned ATTACK tower. The box COLOUR identifies the tower TYPE
    // (towers.csv def id, palette in TowerHUD.cpp); the tower type name sits
    // centred, a small square in the bottom-LEFT corner shows the equipped
    // WEAPON's colour, and a "Lv.N" badge sits bottom-right. A box greys to a
    // red "CD" badge while that tower is on destroy-cooldown (benched until the
    // next round). The data is gathered each frame from the live "Tower" scene
    // objects (placed) plus TowerManager's reserve (unplaced / on cooldown).
    // Heal towers carry no weapon/level so they're not shown here.
    class GAME_DLL TowerHUD : public Engine::UIControl
    {
    public:
        TowerHUD();
        virtual ~TowerHUD() override = default;

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        // Attack towers can never exceed the combined tower cap, so it doubles
        // as the row width.
        static constexpr int kSlotCount = kMaxTowers;

        std::shared_ptr<Engine::Font> m_pNameFont;
        std::shared_ptr<Engine::Font> m_pLvlFont;

        std::shared_ptr<Engine::Button> m_pBoxes[kSlotCount];       // type-coloured background
        std::shared_ptr<Engine::Button> m_pWeaponDots[kSlotCount];  // bottom-left weapon-colour square
        std::shared_ptr<Engine::Text>   m_pNameTexts[kSlotCount];   // tower type name (centred)
        std::shared_ptr<Engine::Text>   m_pLvlTexts[kSlotCount];

        // Change-detection: only re-bind the texture / re-rasterise the text on
        // slots whose tower type, weapon id, level OR state changed this frame.
        // State: -1 empty, 0 placed, 1 ready, 2 on destroy-cooldown.
        int m_iLastTowerIds[kSlotCount];
        int m_iLastIds[kSlotCount];
        int m_iLastLevels[kSlotCount];
        int m_iLastStates[kSlotCount];
    };
}

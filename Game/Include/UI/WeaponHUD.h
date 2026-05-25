#pragma once

#include "UI/UIControl.h"
#include "../Object/Player.h"
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
    // Top-left HUD: a horizontal row of square slot boxes, one per
    // owned weapon (capacity = Player::GetMaxWeaponSlots, currently 6).
    // Each box shows the weapon name centred (DirectWrite wraps to two
    // lines when the name is long) and a small white "Lv.N" tag in
    // the bottom-right corner. Polls Player::GetOwnedWeaponIds +
    // GetOwnedWeaponLevel every frame; only re-applies the texture or
    // SetString on slots whose id/level actually changed so the baked
    // glyph cache inside Engine::Text doesn't re-render each frame.
    class GAME_DLL WeaponHUD : public Engine::UIControl
    {
    public:
        WeaponHUD();
        virtual ~WeaponHUD() override = default;

        void SetTarget(const std::weak_ptr<Player>& pPlayer) { m_pTarget = pPlayer; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        static constexpr int kSlotCount = Player::GetMaxWeaponSlots();

        std::weak_ptr<Player> m_pTarget;
        std::shared_ptr<Engine::Font> m_pNameFont;
        std::shared_ptr<Engine::Font> m_pLvlFont;

        std::shared_ptr<Engine::Button> m_pBoxes[kSlotCount];
        std::shared_ptr<Engine::Text>   m_pNameTexts[kSlotCount];
        std::shared_ptr<Engine::Text>   m_pLvlTexts[kSlotCount];

        int m_iLastIds[kSlotCount];
        int m_iLastLevels[kSlotCount];
    };
}

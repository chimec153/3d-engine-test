#include "TowerHUD.h"
#include "../Object/Tower.h"
#include "../Object/TowerManager.h"
#include "../Object/TowerPlacementController.h"
#include "../Object/TowerData.h"
#include "../Object/TowerSlots.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "UI/Button.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Core/Window.h"
#include "Core/Macro.h"
#include "Input/Input.h"
#include "../Object/GameStateManager.h"
#include "HudTooltip.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Types.h"
#include <algorithm>
#include <string>

namespace Client
{
    namespace TowerHUD_detail
    {
        // Same box geometry as the WeaponHUD, but anchored to the TOP-RIGHT
        // corner (the weapon HUD owns the top-left). The row is right-aligned:
        // its right edge sits kRightFrac in from the window's right edge.
        constexpr float kRightFrac    = 0.020f; // margin from the right edge
        constexpr float kTopFrac      = 0.025f; // top corner
        constexpr float kSlotSizeFrac = 0.085f; // fraction of window height (square)
        constexpr float kSlotGapFrac  = 0.008f; // fraction of window width

        // R-first byte order for the 1x1 RGBA upload (matches WeaponHUD).
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Engine::Texture> EnsureSolidTexture(
            const std::string& strTag, unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>(strTag.c_str())) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>(strTag.c_str());
            if (!pNew) return nullptr;
            unsigned int uColor = PackABGR(uRGB, uAlpha);
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uColor;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        // Per tower-TYPE colour. towers.csv has no colour column, so distinct
        // types are coloured from a fixed palette keyed by the type (def) id.
        // -1 (the default attack type) keeps the base tower blue, matching
        // Tower::Init's diffuse so a default tower's box reads the same colour.
        unsigned int TypeColor(int iTowerDefId)
        {
            if (iTowerDefId < 0) return 0x3A78D8u;
            static const unsigned int kPal[] = {
                0x4FA0FFu, 0xFF6B4Fu, 0x6BD66Bu, 0xC36BFFu, 0xFFD24Fu,
                0x4FE0D6u, 0xFF7FB0u, 0x9AD24Fu, 0xB0855Au, 0x8A8AFFu,
            };
            const int n = static_cast<int>(sizeof(kPal) / sizeof(kPal[0]));
            return kPal[iTowerDefId % n];
        }

        // Box background = the tower type's colour. Ready (owned, not yet on the
        // field) is slightly translucent so it reads as "available to deploy".
        std::shared_ptr<Engine::Texture> TypeBgTexture(int iTowerDefId, int iState)
        {
            const unsigned int uAlpha = (iState == 1) ? 0xC0u : 0xFFu;
            std::string strTag = "tower_hud_type_" + std::to_string(iTowerDefId) + "_" + std::to_string(iState);
            return EnsureSolidTexture(strTag, TypeColor(iTowerDefId), uAlpha);
        }

        // Small bottom-left square = the equipped weapon's colour.
        std::shared_ptr<Engine::Texture> WeaponDotTexture(int iWeaponId, unsigned int uRGB)
        {
            std::string strTag = "tower_hud_wdot_" + std::to_string(iWeaponId);
            return EnsureSolidTexture(strTag, uRGB);
        }

        std::shared_ptr<Engine::Texture> EmptySlotTexture()
        {
            return EnsureSolidTexture("tower_hud_bg_empty", 0x222222, 0x80);
        }

        // Dark red wash for a tower on destroy-cooldown (can't be re-placed
        // until next round).
        std::shared_ptr<Engine::Texture> DownSlotTexture()
        {
            return EnsureSolidTexture("tower_hud_bg_down", 0x401818, 0xC0);
        }

        // Heal-tower box = green (heal towers carry no per-type colour). Ready
        // (unplaced) is slightly translucent, matching the attack TypeBgTexture.
        std::shared_ptr<Engine::Texture> HealSlotTexture(int iState)
        {
            const unsigned int uAlpha = (iState == 1) ? 0xC0u : 0xFFu;
            std::string strTag = "tower_hud_heal_" + std::to_string(iState);
            return EnsureSolidTexture(strTag, 0x2FB85Au, uAlpha);
        }
    }

    TowerHUD::TowerHUD()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
        for (int i = 0; i < kSlotCount; ++i)
        {
            m_iLastTowerIds[i] = -2;   // -2 so the first frame always refreshes
            m_iLastIds[i]      = -1;
            m_iLastLevels[i]   = -1;
            m_iLastStates[i]   = -1;
            m_iLastHeal[i]     = -1;
            m_iLastWpnLvl[i]   = -1;
            m_iReserveIdx[i]   = -1;
            m_bHealDeploy[i]   = false;
        }
    }

    bool TowerHUD::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        const float fSlotSize = (std::max)(48.f, fScreenH * TowerHUD_detail::kSlotSizeFrac);
        const float fSlotGap  = (std::max)( 4.f, fScreenW * TowerHUD_detail::kSlotGapFrac);
        // Right-align the whole row in the top-right corner.
        const float fTotalW   = kSlotCount * fSlotSize + (kSlotCount - 1) * fSlotGap;
        const float fLeft     = fScreenW - fScreenW * TowerHUD_detail::kRightFrac - fTotalW;
        const float fTop      = fScreenH * TowerHUD_detail::kTopFrac;

        const float fNameSize = (std::max)(11.f, fSlotSize * 0.16f);
        const float fLvlSize  = (std::max)(10.f, fSlotSize * 0.18f);

        m_pNameFont = Engine::FontManager::GetInst()->CreateFont(
            "tower_hud_name", L"Arial", fNameSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pLvlFont = Engine::FontManager::GetInst()->CreateFont(
            "tower_hud_lvl",  L"Arial", fLvlSize,  DWRITE_FONT_WEIGHT_BOLD);

        m_fBoxY    = fTop;
        m_fBoxSize = fSlotSize;
        for (int i = 0; i < kSlotCount; ++i)
        {
            const float fX = fLeft + i * (fSlotSize + fSlotGap);
            m_fBoxX[i] = fX;   // cached for the hover hit-test

            std::string tagBox = "btn_tower_hud_" + std::to_string(i);
            m_pBoxes[i] = CreateComponent<Engine::Button>(tagBox);
            if (m_pBoxes[i])
            {
                m_pBoxes[i]->SetRect(fX, fTop, fSlotSize, fSlotSize);
                m_pBoxes[i]->SetTexture(TowerHUD_detail::EmptySlotTexture());
                // Clicking a box deploys the SPECIFIC tower it shows (mapping
                // refreshed each frame in Update): a ready heal slot starts heal
                // placement, a ready attack slot starts attack placement of that
                // reserve. Non-deployable slots (placed / cooldown / weaponless /
                // empty) harmlessly do nothing.
                m_pBoxes[i]->SetOnClick([this, i]
                {
                    auto p = m_pPlacement.lock();
                    if (!p) return;
                    if (m_bHealDeploy[i])           p->BeginHealPlacement();
                    else if (m_iReserveIdx[i] >= 0) p->BeginAttackPlacement(m_iReserveIdx[i]);
                });
            }

            // Weapon-colour square, bottom-left corner. Created after the box so
            // it draws on top of it; the texts (created next) draw on top again.
            std::string tagDot = "btn_tower_hud_wdot_" + std::to_string(i);
            m_pWeaponDots[i] = CreateComponent<Engine::Button>(tagDot);
            const float fDot = fSlotSize * 0.30f;
            const float fPad = fSlotSize * 0.06f;
            const float fDotX = fX + fPad, fDotY = fTop + fSlotSize - fDot - fPad;
            m_fDotX[i] = fDotX; m_fDotY = fDotY; m_fDotSize = fDot;
            if (m_pWeaponDots[i])
            {
                m_pWeaponDots[i]->SetRect(fDotX, fDotY, fDot, fDot);
                m_pWeaponDots[i]->SetTexture(TowerHUD_detail::EmptySlotTexture());
                m_pWeaponDots[i]->Disable();   // shown only when the slot is filled
            }
            // The equipped WEAPON's level, drawn on top of the weapon dot.
            std::string tagWL = "text_tower_hud_wlvl_" + std::to_string(i);
            m_pWpnLvlTexts[i] = CreateComponent<Engine::Text>(tagWL);
            if (m_pWpnLvlTexts[i])
            {
                m_pWpnLvlTexts[i]->SetFont(m_pLvlFont);
                m_pWpnLvlTexts[i]->SetColor(0xFFFFFFFFu);
                m_pWpnLvlTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pWpnLvlTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pWpnLvlTexts[i]->SetRect(fDotX, fDotY, fDot, fDot);
                m_pWpnLvlTexts[i]->Disable();
            }

            // Slot number "1".."5" centred just BELOW the icon — the key that
            // deploys this slot's tower. Hidden when the slot is empty (Update).
            std::string tagNum = "text_tower_hud_num_" + std::to_string(i);
            m_pNumTexts[i] = CreateComponent<Engine::Text>(tagNum);
            if (m_pNumTexts[i])
            {
                m_pNumTexts[i]->SetFont(m_pLvlFont);
                m_pNumTexts[i]->SetColor(0xFFE070FFu);   // warm gold = hotkey
                m_pNumTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pNumTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                const float fNumH = (std::max)(14.f, fSlotSize * 0.26f);
                m_pNumTexts[i]->SetRect(fX, fTop + fSlotSize + fSlotGap, fSlotSize, fNumH);
                m_pNumTexts[i]->SetString(std::to_wstring(i + 1));
                m_pNumTexts[i]->Disable();
            }

            // Placement-status badge — a short caption along the TOP of the box
            // (PLACED / READY / CD) so the player can tell at a glance whether a
            // tower is on the field, waiting to deploy, or benched on cooldown.
            std::string tagStatus = "text_tower_hud_status_" + std::to_string(i);
            m_pStatusTexts[i] = CreateComponent<Engine::Text>(tagStatus);
            if (m_pStatusTexts[i])
            {
                m_pStatusTexts[i]->SetFont(m_pLvlFont);
                m_pStatusTexts[i]->SetColor(0xFFFFFFFFu);
                m_pStatusTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pStatusTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                const float fInsetX = fSlotSize * 0.06f;
                m_pStatusTexts[i]->SetRect(
                    fX + fInsetX,
                    fTop + fSlotSize * 0.05f,
                    fSlotSize - 2.f * fInsetX,
                    fSlotSize * 0.22f);
            }

            std::string tagName = "text_tower_hud_name_" + std::to_string(i);
            m_pNameTexts[i] = CreateComponent<Engine::Text>(tagName);
            if (m_pNameTexts[i])
            {
                m_pNameTexts[i]->SetFont(m_pNameFont);
                m_pNameTexts[i]->SetColor(0xFFFFFFFFu);
                m_pNameTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pNameTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                const float fInsetX = fSlotSize * 0.06f;
                // Pushed below the status badge (top ~0.27) so the two don't overlap.
                m_pNameTexts[i]->SetRect(
                    fX + fInsetX,
                    fTop + fSlotSize * 0.28f,
                    fSlotSize - 2.f * fInsetX,
                    fSlotSize * 0.46f);
            }

            std::string tagLvl = "text_tower_hud_lvl_" + std::to_string(i);
            m_pLvlTexts[i] = CreateComponent<Engine::Text>(tagLvl);
            if (m_pLvlTexts[i])
            {
                m_pLvlTexts[i]->SetFont(m_pLvlFont);
                m_pLvlTexts[i]->SetColor(0xFFFFFFFFu);
                m_pLvlTexts[i]->SetHAlign(Engine::Text::HAlign::Right);
                m_pLvlTexts[i]->SetVAlign(Engine::Text::VAlign::Bottom);
                const float fLvlW = fSlotSize * 0.55f;
                const float fLvlH = fSlotSize * 0.26f;
                const float fPad  = fSlotSize * 0.04f;
                m_pLvlTexts[i]->SetRect(
                    fX + fSlotSize - fLvlW - fPad,
                    fTop + fSlotSize - fLvlH - fPad,
                    fLvlW,
                    fLvlH);
            }
        }

        // (Per-slot number captions below each icon replace the old single
        // key-press hint — each box shows its own deploy key, set in Update.)

        // Hover tooltip panel (created last so it draws above the slots).
        m_pTipBg = CreateComponent<Engine::Button>("tower_hud_tip_bg");
        if (m_pTipBg)
        {
            m_pTipBg->SetTexture(TowerHUD_detail::EnsureSolidTexture("tower_hud_tip_bg", 0x101014, 0xE0));
            m_pTipBg->Disable();
        }
        m_pTipText = CreateComponent<Engine::Text>("tower_hud_tip_text");
        if (m_pTipText)
        {
            m_pTipText->SetFont(m_pNameFont);
            m_pTipText->SetColor(0xFFFFFFFFu);
            m_pTipText->SetHAlign(Engine::Text::HAlign::Left);
            m_pTipText->SetVAlign(Engine::Text::VAlign::Top);
            m_pTipText->Disable();
        }
        return true;
    }

    void TowerHUD::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);
        using namespace TowerHUD_detail;

        // Shared ordered slot list (acquisition order, attack + heal interleaved)
        // — identical to the list the placement controller maps the number keys
        // onto, so slot N here == key N there. State: 0 placed, 1 ready, 2 down.
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        const std::vector<TowerSlotView> slots = BuildTowerSlots(pLayer.get());
        const int n = static_cast<int>(slots.size());

        for (int i = 0; i < kSlotCount; ++i)
        {
            const bool bHas    = i < n;
            const bool bHeal   = bHas && slots[i].bHeal;
            const int iTowerId = bHas ? slots[i].iTowerId  : -1;
            const int iId      = bHas ? slots[i].iWeaponId : -1;
            const int iLevel   = bHas ? slots[i].iLevel       : -1;   // tower level
            const int iState   = bHas ? slots[i].eState       : -1;
            const int iWpnLvl  = bHas ? slots[i].iWeaponLevel : -1;   // weapon's level

            // Refresh the click->deploy mapping every frame (the slot order can
            // shift without this slot's displayed content changing), so it can't
            // go stale behind the change-detection skip below.
            m_iReserveIdx[i] = bHas ? slots[i].iReserveIdx : -1;
            m_bHealDeploy[i] = bHas && bHeal && slots[i].Deployable();

            const int iHealKey = bHeal ? 1 : 0;
            if (iTowerId == m_iLastTowerIds[i] && iId == m_iLastIds[i] &&
                iLevel == m_iLastLevels[i] && iState == m_iLastStates[i] &&
                iHealKey == m_iLastHeal[i] && iWpnLvl == m_iLastWpnLvl[i])
                continue;
            m_iLastTowerIds[i] = iTowerId;
            m_iLastIds[i]      = iId;
            m_iLastLevels[i]   = iLevel;
            m_iLastStates[i]   = iState;
            m_iLastHeal[i]     = iHealKey;
            m_iLastWpnLvl[i]   = iWpnLvl;

            // Slot number badge: shown (= deploy key) only on filled slots.
            if (m_pNumTexts[i]) { if (bHas) m_pNumTexts[i]->Enable(); else m_pNumTexts[i]->Disable(); }

            if (!bHas)
            {
                if (m_pBoxes[i])       m_pBoxes[i]->SetTexture(EmptySlotTexture());
                if (m_pWeaponDots[i])  m_pWeaponDots[i]->Disable();
                if (m_pWpnLvlTexts[i]) m_pWpnLvlTexts[i]->Disable();
                if (m_pNameTexts[i])   m_pNameTexts[i]->SetString(L"");
                if (m_pStatusTexts[i]) m_pStatusTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])    m_pLvlTexts[i]->SetString(L"");
                continue;
            }

            // Name: heal towers carry no type, so they get a fixed label; attack
            // towers resolve their towers.csv type name (id < 0 = default type).
            std::wstring wsType;
            if (bHeal)
                wsType = L"Heal Tower";
            else
            {
                const TowerDef* pTD = (iTowerId >= 0)
                    ? TowerDatabase::GetInst().Get(iTowerId)
                    : TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
                wsType = pTD ? std::wstring(pTD->strName.begin(), pTD->strName.end())
                             : std::wstring(L"Tower");
            }

            // Equipped WEAPON -> bottom-left square (attack only; a weaponless or
            // heal tower shows NO square so it reads clearly as unequipped).
            if (m_pWeaponDots[i])
            {
                if (!bHeal && iId >= 0)
                {
                    const WeaponDef* pWDef = WeaponDatabase::GetInst().Get(iId);
                    const unsigned int uWeaponColor = pWDef ? pWDef->uColorRGB : 0x606060u;
                    m_pWeaponDots[i]->SetTexture(WeaponDotTexture(iId, uWeaponColor));
                    m_pWeaponDots[i]->Enable();
                }
                else m_pWeaponDots[i]->Disable();
            }
            // The equipped weapon's LEVEL, on the dot (attack + armed only).
            if (m_pWpnLvlTexts[i])
            {
                if (!bHeal && iId >= 0 && iWpnLvl > 0)
                {
                    m_pWpnLvlTexts[i]->SetColor(iState == 2 ? 0x909090FFu : 0xFFFFFFFFu);
                    m_pWpnLvlTexts[i]->SetString(std::to_wstring(iWpnLvl));
                    m_pWpnLvlTexts[i]->Enable();
                }
                else m_pWpnLvlTexts[i]->Disable();
            }

            // Box colour: cooldown = red wash; heal = green; attack = type colour.
            if (m_pBoxes[i])
            {
                if      (iState == 2) m_pBoxes[i]->SetTexture(DownSlotTexture());
                else if (bHeal)       m_pBoxes[i]->SetTexture(HealSlotTexture(iState));
                else                  m_pBoxes[i]->SetTexture(TypeBgTexture(iTowerId, iState));
            }

            // Name colour (greyed while benched).
            if (m_pNameTexts[i])
            {
                m_pNameTexts[i]->SetColor(iState == 2 ? 0x909090FFu : 0xFFFFFFFFu);
                m_pNameTexts[i]->SetString(wsType);
            }

            // Status badge: PLACED (on field) / READY / NO WPN (weaponless attack,
            // can't deploy) / CD (benched on destroy-cooldown).
            if (m_pStatusTexts[i])
            {
                if      (iState == 2)            { m_pStatusTexts[i]->SetColor(0xFF6060FFu); m_pStatusTexts[i]->SetString(L"CD"); }
                else if (iState == 0)            { m_pStatusTexts[i]->SetColor(0x90FFA0FFu); m_pStatusTexts[i]->SetString(L"PLACED"); }
                else if (!bHeal && iId < 0)      { m_pStatusTexts[i]->SetColor(0xFFB040FFu); m_pStatusTexts[i]->SetString(L"NO WPN"); }
                else                             { m_pStatusTexts[i]->SetColor(0xC0E0FFFFu); m_pStatusTexts[i]->SetString(L"READY"); }
            }

            // Level badge — tower level. Heal towers show it too now (they level).
            if (m_pLvlTexts[i])
            {
                m_pLvlTexts[i]->SetColor(
                    iState == 2 ? 0x909090FFu : (iState == 0 ? 0xFFFFFFFFu : 0xC0E0FFFFu));
                m_pLvlTexts[i]->SetString(L"Lv." + std::to_wstring(iLevel));
            }
        }

        // --- Hover tooltip: weapon dot → weapon stats; box → tower stats. -------
        auto hideTip = [&]() {
            if (m_pTipBg)   m_pTipBg->Disable();
            if (m_pTipText) m_pTipText->Disable();
        };
        // Tooltip shows on hover in EVERY state (play + any modal); it only
        // hides when the cursor isn't over a slot (handled below).

        auto* pInput = Engine::CInput::GetInst();
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());
        auto inRect = [&](float x, float y, float w, float h)
        { return mx >= x && mx < x + w && my >= y && my < y + h; };

        std::wstring wInfo;
        for (int i = 0; i < kSlotCount; ++i)
        {
            if (m_iLastStates[i] < 0) continue;   // empty slot
            const bool bHeal = (m_iLastHeal[i] == 1);
            // Inner weapon dot first → the equipped weapon's stats (attack only).
            if (!bHeal && m_iLastIds[i] >= 0 &&
                inRect(m_fDotX[i], m_fDotY, m_fDotSize, m_fDotSize))
            {
                if (const WeaponDef* pW = WeaponDatabase::GetInst().Get(m_iLastIds[i]))
                    wInfo = HudTip::Weapon(*pW, m_iLastWpnLvl[i]);
                break;
            }
            // Otherwise the box → tower stats (heal vs attack).
            if (inRect(m_fBoxX[i], m_fBoxY, m_fBoxSize, m_fBoxSize))
            {
                if (bHeal)
                    wInfo = HudTip::HealStats(TowerDatabase::GetInst().FirstOfKind(TowerKind::Heal), m_iLastLevels[i]);
                else
                {
                    const TowerDef* pTD = (m_iLastTowerIds[i] >= 0)
                        ? TowerDatabase::GetInst().Get(m_iLastTowerIds[i])
                        : TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
                    wInfo = HudTip::TowerStats(pTD, m_iLastLevels[i], m_iLastIds[i], m_iLastWpnLvl[i]);
                }
                break;
            }
        }
        if (wInfo.empty()) { hideTip(); return; }

        const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());
        int iLines = 1; for (wchar_t c : wInfo) if (c == L'\n') ++iLines;
        const float fPad   = 6.f;
        const float fLineH = (std::max)(14.f, m_fBoxSize * 0.16f);
        const float fTipW  = m_fBoxSize * 3.0f;
        const float fTipH  = iLines * fLineH + fPad * 2.f;
        float tx = mx + 18.f, ty = my + 14.f;
        if (tx + fTipW > fScreenW) tx = mx - fTipW - 18.f;
        if (tx < 0.f) tx = 0.f;
        if (ty + fTipH > fScreenH) ty = fScreenH - fTipH;
        if (ty < 0.f) ty = 0.f;
        if (m_pTipBg)   { m_pTipBg->SetRect(tx, ty, fTipW, fTipH); m_pTipBg->Enable(); }
        if (m_pTipText)
        {
            m_pTipText->SetRect(tx + fPad, ty + fPad, fTipW - fPad * 2.f, fTipH - fPad * 2.f);
            m_pTipText->SetString(wInfo);
            m_pTipText->Enable();
        }
    }

    std::shared_ptr<Engine::Component> TowerHUD::Clone()
    {
        return std::make_shared<TowerHUD>(*this);
    }
}

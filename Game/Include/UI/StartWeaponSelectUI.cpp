#include "StartWeaponSelectUI.h"
#include "UI/Button.h"
#include "../Object/Player.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/GameStateManager.h"
#include "../Object/TowerManager.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Input/Input.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include <algorithm>
#include <string>
#include <vector>

namespace Client
{
    namespace StartWeaponSelectUI_detail
    {
        struct Layout
        {
            float fW, fItemH, fGap, fTitleH, fLeftX, fTopY, fItemsY;
        };

        Layout ComputeLayout()
        {
            const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
            const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

            Layout L;
            L.fW      = 0.30f * W;
            L.fItemH  = (std::max)(30.f, 0.05f * H);
            L.fGap    = (std::max)(6.f, 0.012f * H);
            L.fTitleH = (std::max)(34.f, 0.07f * H);
            L.fLeftX  = (W - L.fW) * 0.5f;
            L.fTopY   = 0.12f * H;
            L.fItemsY = L.fTopY + L.fTitleH + L.fGap;
            return L;
        }

        // ABGR memory layout (bytes R,G,B,A) — matches the other UI panels.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Engine::Texture> EnsureSolidTexture(const std::string& strTag, unsigned int uRGB)
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>(strTag.c_str())) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>(strTag.c_str());
            if (!pNew) return nullptr;
            unsigned int uColor = PackABGR(uRGB);
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uColor;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        std::wstring ToW(const std::string& s) { return std::wstring(s.begin(), s.end()); }

        // Short, human labels for the WeaponDef enums shown in the tooltip.
        // (Mirrors TowerIntermissionUI's shop tooltip so both panels read the
        // same; duplicated to keep each UI file self-contained, like PackABGR.)
        const wchar_t* MoveName(MovementType e)
        {
            switch (e)
            {
            case MovementType::Straight: return L"Straight";
            case MovementType::Spiral:   return L"Spiral";
            case MovementType::Fixed:    return L"Fixed";
            case MovementType::Orbital:  return L"Orbital";
            case MovementType::Homing:   return L"Homing";
            case MovementType::Aimed:    return L"Aimed";
            case MovementType::Follow:   return L"Follow (pet)";
            default:                     return L"?";
            }
        }
        const wchar_t* HitName(OnHitEvent e)
        {
            switch (e)
            {
            case OnHitEvent::Vanish:   return L"Vanish";
            case OnHitEvent::NoChange: return L"Pierce";
            case OnHitEvent::Reflect:  return L"Bounce";
            case OnHitEvent::Multiply: return L"Split";
            case OnHitEvent::Field:    return L"Zone";
            case OnHitEvent::Chain:    return L"Chain";
            default:                   return L"?";
            }
        }
        const wchar_t* AimName(AimMode e)
        {
            switch (e)
            {
            case AimMode::Nearest:  return L"Nearest";
            case AimMode::LowestHP: return L"LowestHP";
            case AimMode::Random:   return L"Random";
            case AimMode::Forward:  return L"Forward";
            case AimMode::Cursor:   return L"Cursor";
            case AimMode::Radial:   return L"Radial";
            default:                return L"?";
            }
        }
        // One-decimal format without <sstream> / locale.
        std::wstring F1(float v)
        {
            const int t = static_cast<int>((v < 0.f ? -v : v) * 10.f + 0.5f);
            std::wstring s = std::to_wstring(t / 10) + L"." + std::to_wstring(t % 10);
            return v < 0.f ? (L"-" + s) : s;
        }

        // Build the multi-line tooltip body for a weapon at a given level.
        std::wstring BuildWeaponTooltip(const WeaponDef& def, int iLevel)
        {
            if (iLevel < 1) iLevel = 1;
            const int   dmg = ComputeDamage(def, iLevel);
            const int   cnt = ComputeCount (def, iLevel);
            const float cd  = ComputeCooldown(def, iLevel);
            const float spd = ComputeSpeed (def, iLevel);
            const float sz  = ComputeSize  (def, iLevel);

            std::wstring s = ToW(def.strName) + L"  Lv." + std::to_wstring(iLevel) + L"\n";
            s += L"DMG " + std::to_wstring(dmg);
            if (cnt > 1) s += L"  x" + std::to_wstring(cnt);
            s += L"\n";
            if (def.eFireMode == FireMode::Sustained)
                s += L"Sustained";
            else
                s += L"Cooldown " + F1(cd) + L"s";
            if (def.fDamageInterval > 0.f) s += L"  tick " + F1(def.fDamageInterval) + L"s";
            s += L"\n";
            s += std::wstring(L"Move ") + MoveName(def.eMovement) +
                 L"  Hit " + HitName(def.eOnHit) + L"\n";
            s += std::wstring(L"Aim ") + AimName(def.eAimMode) +
                 L"  Spd " + F1(spd) + L"  Size " + F1(sz) + L"\n";
            s += L"Life " + F1(def.fLifetime) + L"s";
            if (def.iMaxHits > 0) s += L"  MaxHits " + std::to_wstring(def.iMaxHits);
            s += L"\n";

            // Impact modules beyond the always-on Damage baseline.
            std::wstring fx;
            const unsigned int m = def.uImpactMask;
            if (m & Impact_Knockback) fx += (fx.empty() ? L"" : L", ") + std::wstring(L"Knockback");
            if (m & Impact_Gather)    fx += (fx.empty() ? L"" : L", ") + std::wstring(L"Pull");
            if (m & Impact_Burn)      fx += (fx.empty() ? L"" : L", ") + std::wstring(L"Burn " + std::to_wstring(def.iBurnDamage));
            if (m & Impact_Slow)      fx += (fx.empty() ? L"" : L", ") + std::wstring(L"Slow");
            if (!fx.empty()) s += L"FX: " + fx + L"\n";

            if (def.iEvolvesInto > 0)
                s += L"Evolves at Lv." + std::to_wstring(def.iEvolveMinLevel);

            return s;
        }
    }

    StartWeaponSelectUI::StartWeaponSelectUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
        for (int i = 0; i < kMaxItems; ++i) m_iItemWeaponIds[i] = -1;
    }

    bool StartWeaponSelectUI::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        const auto L = StartWeaponSelectUI_detail::ComputeLayout();

        const float fTitleSize = (std::max)(20.f, L.fTitleH * 0.55f);
        const float fItemSize  = (std::max)(14.f, L.fItemH  * 0.50f);
        m_pTitleFont = Engine::FontManager::GetInst()->CreateFont(
            "start_sel_title", L"Arial", fTitleSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pItemFont = Engine::FontManager::GetInst()->CreateFont(
            "start_sel_item", L"Arial", fItemSize, DWRITE_FONT_WEIGHT_BOLD);

        m_pTitle = CreateComponent<Engine::Text>("start_sel_title_text");
        if (m_pTitle)
        {
            m_pTitle->SetFont(m_pTitleFont);
            m_pTitle->SetColor(0xFFFFFFFFu);
            m_pTitle->SetHAlign(Engine::Text::HAlign::Center);
            m_pTitle->SetVAlign(Engine::Text::VAlign::Center);
            m_pTitle->SetRect(L.fLeftX, L.fTopY, L.fW, L.fTitleH);
            m_pTitle->SetString(L"Choose Your Starting Weapon");
        }

        for (int i = 0; i < kMaxItems; ++i)
        {
            const float fY = L.fItemsY + i * (L.fItemH + L.fGap);

            m_ItemRect[i] = { L.fLeftX, fY, L.fW, L.fItemH };

            m_pItemButtons[i] = CreateComponent<Engine::Button>("start_sel_item_" + std::to_string(i));
            if (m_pItemButtons[i])
            {
                m_pItemButtons[i]->SetRect(L.fLeftX, fY, L.fW, L.fItemH);
                m_pItemButtons[i]->SetTexture(
                    StartWeaponSelectUI_detail::EnsureSolidTexture("start_sel_blank", 0x303030));
                const int idx = i;
                m_pItemButtons[i]->SetOnClick([this, idx]() { OnPick(idx); });
            }

            m_pItemTexts[i] = CreateComponent<Engine::Text>("start_sel_item_text_" + std::to_string(i));
            if (m_pItemTexts[i])
            {
                m_pItemTexts[i]->SetFont(m_pItemFont);
                m_pItemTexts[i]->SetColor(0xFFFFFFFFu);
                m_pItemTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pItemTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pItemTexts[i]->SetRect(L.fLeftX, fY, L.fW, L.fItemH);
            }
        }

        // Tooltip panel + text, created last so they draw above the rows. Rect
        // is set per-frame by HandleTooltip; hidden until a hover.
        m_pTooltipBg = CreateComponent<Engine::Button>("start_sel_tip_bg");
        if (m_pTooltipBg)
            m_pTooltipBg->SetTexture(
                StartWeaponSelectUI_detail::EnsureSolidTexture("start_sel_tip_bg_tex", 0x12141C));
        m_pTooltipText = CreateComponent<Engine::Text>("start_sel_tip_t");
        if (m_pTooltipText)
        {
            m_pTooltipText->SetFont(m_pItemFont);
            m_pTooltipText->SetColor(0xEDEFF5FFu);
            m_pTooltipText->SetHAlign(Engine::Text::HAlign::Left);
            m_pTooltipText->SetVAlign(Engine::Text::VAlign::Top);
        }

        Hide();
        return true;
    }

    void StartWeaponSelectUI::Show()
    {
        if (m_pTitle) m_pTitle->Enable();
        // Rows enabled selectively by BuildList.
    }

    void StartWeaponSelectUI::Hide()
    {
        if (m_pTitle) m_pTitle->Disable();
        for (int i = 0; i < kMaxItems; ++i)
        {
            if (m_pItemButtons[i]) m_pItemButtons[i]->Disable();
            if (m_pItemTexts[i])   m_pItemTexts[i]->Disable();
        }
        if (m_pTooltipBg)   m_pTooltipBg->Disable();
        if (m_pTooltipText) m_pTooltipText->Disable();
    }

    void StartWeaponSelectUI::BuildList()
    {
        // The start picker pool, first kMaxItems in deterministic order: the
        // round-1 shop-available weapons plus every weapon the player has
        // acquired (unlocked) in a past run -- so a later-round weapon you've
        // earned can be chosen as a starting weapon.
        const std::vector<int> vecIds = WeaponDatabase::GetInst().StartWeaponIds();
        m_iCount = (std::min)(kMaxItems, static_cast<int>(vecIds.size()));

        for (int i = 0; i < kMaxItems; ++i)
        {
            const bool bHas = i < m_iCount;
            const int  id   = bHas ? vecIds[i] : -1;
            m_iItemWeaponIds[i] = id;

            if (!bHas)
            {
                if (m_pItemButtons[i]) m_pItemButtons[i]->Disable();
                if (m_pItemTexts[i])   m_pItemTexts[i]->Disable();
                continue;
            }

            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            if (m_pItemButtons[i])
            {
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pItemButtons[i]->SetTexture(StartWeaponSelectUI_detail::EnsureSolidTexture(
                    "start_sel_w_" + std::to_string(id), uColor));
                m_pItemButtons[i]->Enable();
            }
            if (m_pItemTexts[i])
            {
                const std::wstring wName = pDef
                    ? std::wstring(pDef->strName.begin(), pDef->strName.end())
                    : L"Weapon";
                m_pItemTexts[i]->SetString(wName);
                m_pItemTexts[i]->Enable();
            }
        }
    }

    void StartWeaponSelectUI::OnPick(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::StartSelect) return;
        if (iIndex < 0 || iIndex >= m_iCount) return;
        const int id = m_iItemWeaponIds[iIndex];
        if (id < 0) return;

        // Arm both the player (a loadout slot) and the towers with the pick.
        if (auto pPlayer = m_pTarget.lock())
            pPlayer->AddOrLevelUpWeapon(id);
        TowerManager::GetInst().SetCurrentWeaponId(id);

        Hide();
        m_bShownLocal = false;
        if (m_fnChosen) m_fnChosen();   // GameScene: StartRound(1) + ExitModal
    }

    void StartWeaponSelectUI::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        const bool bSelect =
            GameStateManager::GetInst().GetState() == GameState::StartSelect;

        if (bSelect && !m_bShownLocal)
        {
            BuildList();
            Show();
            m_bShownLocal = true;
        }
        else if (!bSelect && m_bShownLocal)
        {
            Hide();
            m_bShownLocal = false;
        }

        if (m_bShownLocal)
            HandleTooltip();
    }

    void StartWeaponSelectUI::HandleTooltip()
    {
        using namespace StartWeaponSelectUI_detail;

        auto hideTip = [&]()
        {
            if (m_pTooltipBg)   m_pTooltipBg->Disable();
            if (m_pTooltipText) m_pTooltipText->Disable();
        };

        auto* pInput = Engine::CInput::GetInst();
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());

        // What's under the cursor? Only the enabled weapon rows. Stats read at
        // level 1 — these are the starting (unowned) weapons.
        std::wstring wInfo;
        for (int i = 0; i < m_iCount; ++i)
        {
            if (m_iItemWeaponIds[i] < 0 || !InRect(mx, my, m_ItemRect[i])) continue;
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iItemWeaponIds[i]);
            if (pDef) wInfo = BuildWeaponTooltip(*pDef, 1);
            break;
        }

        if (wInfo.empty()) { hideTip(); return; }

        // Size the panel to the line count; place it next to the cursor and clamp
        // to the window so it never spills off-screen.
        const auto L = ComputeLayout();
        const float Wpx = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float Hpx = static_cast<float>(Engine::Window::GetInst()->GetHeight());
        int iLines = 1;
        for (wchar_t c : wInfo) if (c == L'\n') ++iLines;
        const float fPad   = (std::max)(4.f, L.fGap * 1.5f);
        const float fLineH = (std::max)(14.f, L.fItemH * 0.62f);
        const float fTipW  = L.fW * 0.9f;
        const float fTipH  = iLines * fLineH + fPad * 2.f;
        float tx = mx + 18.f;
        float ty = my + 14.f;
        if (tx + fTipW > Wpx) tx = mx - fTipW - 18.f;   // flip to the left edge
        if (tx < 0.f) tx = 0.f;
        if (ty + fTipH > Hpx) ty = Hpx - fTipH;
        if (ty < 0.f) ty = 0.f;

        if (m_pTooltipBg)
        {
            m_pTooltipBg->SetRect(tx, ty, fTipW, fTipH);
            m_pTooltipBg->Enable();
        }
        if (m_pTooltipText)
        {
            m_pTooltipText->SetRect(tx + fPad, ty + fPad, fTipW - fPad * 2.f, fTipH - fPad * 2.f);
            m_pTooltipText->SetString(wInfo);
            m_pTooltipText->Enable();
        }
    }

    std::shared_ptr<Engine::Component> StartWeaponSelectUI::Clone()
    {
        return std::make_shared<StartWeaponSelectUI>(*this);
    }
}

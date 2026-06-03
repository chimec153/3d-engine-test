#include "TowerIntermissionUI.h"
#include "UI/Button.h"
#include "../Object/Player.h"
#include "../Object/Tower.h"
#include "../Object/HealTower.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/TowerData.h"
#include "../Object/GameStateManager.h"
#include "../Object/TowerManager.h"
#include "../Object/Wallet.h"
#include "../GameDefs.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Transform.h"
#include "Core/Window.h"
#include "Input/Input.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace Client
{
    namespace TowerIntermissionUI_detail
    {
        struct Sizes
        {
            float fW;        // panel / row width
            float fLeftX;    // common left edge
            float fTopY;     // top of the panel
            float fTitleH;
            float fInfoH;
            float fHeaderH;
            float fItemH;
            float fGap;
        };

        Sizes ComputeSizes()
        {
            const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
            const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

            Sizes S;
            S.fW       = 0.30f * W;
            S.fLeftX   = (W - S.fW) * 0.5f;
            S.fTopY    = 0.04f * H;
            S.fTitleH  = (std::max)(28.f, 0.05f * H);
            S.fInfoH   = (std::max)(20.f, 0.032f * H);
            S.fHeaderH = (std::max)(18.f, 0.028f * H);
            S.fItemH   = (std::max)(24.f, 0.04f * H);
            S.fGap     = (std::max)(4.f, 0.006f * H);
            return S;
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

        // Short, human labels for the WeaponDef enums used in the tooltip.
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
            const int   dmg   = ComputeDamage(def, iLevel);
            const int   cnt   = ComputeCount (def, iLevel);
            const float cd    = ComputeCooldown(def, iLevel);
            const float spd   = ComputeSpeed (def, iLevel);
            const float sz    = ComputeSize  (def, iLevel);

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

        // Tower stat lookups (towers.csv) — null when no row is loaded, so each
        // reader falls back to the GameDefs constants.
        const TowerDef* AttackTowerDef() { return TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack); }
        const TowerDef* HealTowerDef()   { return TowerDatabase::GetInst().FirstOfKind(TowerKind::Heal); }

        // Buy / refund price for a tower kind — the CSV value, or the GameDefs
        // constant when no row is loaded.
        int TowerBuyPrice(bool bHeal)
        {
            if (bHeal) { const TowerDef* d = HealTowerDef();   return d ? d->iPrice : kHealTowerPrice; }
            const TowerDef* d = AttackTowerDef();              return d ? d->iPrice : kTowerPrice;
        }

        // Build the tooltip body for a tower buy row. Towers aren't WeaponDefs,
        // so they get a fixed description sourced from towers.csv (attack tower
        // fires its equipped weapon; heal tower pulses HP to nearby allies).
        std::wstring BuildTowerTooltip(bool bHeal)
        {
            if (bHeal)
            {
                const TowerDef* d = HealTowerDef();
                const int   iPrice = d ? d->iPrice        : kHealTowerPrice;
                const int   iHP    = d ? d->iHP           : kHealTowerHP;
                const int   iAmt   = d ? d->iHealAmount   : kHealAmount;
                const float fInt   = d ? d->fHealInterval : kHealInterval;
                const float fRad   = d ? d->fHealRadius   : kHealRadius;
                std::wstring s = L"Heal Tower  $" + std::to_wstring(iPrice) + L"\n";
                s += L"HP " + std::to_wstring(iHP) + L"\n";
                s += L"Pulses +" + std::to_wstring(iAmt) + L" HP every " + F1(fInt) + L"s\n";
                s += L"to allies within " + F1(fRad);
                return s;
            }
            const TowerDef* d = AttackTowerDef();
            const int   iPrice = d ? d->iPrice   : kTowerPrice;
            const int   iHP    = d ? d->iHP      : kTowerHP;
            std::wstring s = L"Tower  $" + std::to_wstring(iPrice) + L"\n";
            s += L"HP " + std::to_wstring(iHP);
            if (d && d->fDefense > 0.f) s += L"  Def " + std::to_wstring(static_cast<int>(d->fDefense * 100.f + 0.5f)) + L"%";
            s += L"\n";
            if (d)
            {
                s += L"Atk x" + F1(d->fAttack) + L"  Spd x" + F1(d->fAttackSpeed) + L"\n";
                if (d->fCritChance > 0.f)
                    s += L"Crit " + std::to_wstring(static_cast<int>(d->fCritChance * 100.f + 0.5f)) +
                         L"% x" + F1(d->fCritMult) + L"\n";
                s += L"Range " + F1(d->fRange) + L"\n";
            }
            s += L"Auto-fires its equipped weapon";
            return s;
        }

        // --- Typed attack-tower helpers (shop buys a specific towers.csv type) -
        // Resolve a buy row's tower type by id (-1 / unknown = default attack
        // type, preserving the pre-type-select rows).
        const TowerDef* TowerDefById(int iId)
        {
            const TowerDef* d = (iId >= 0) ? TowerDatabase::GetInst().Get(iId) : nullptr;
            return d ? d : AttackTowerDef();
        }
        // Which towers.csv kinds are buyable as bullet-firing attack towers
        // (Heal is its own object; Buff is an aura with no weapon fire).
        bool IsBuyableAttackType(const TowerDef& d)
        {
            switch (d.eKind)
            {
            case TowerKind::Attack:
            case TowerKind::Frost:
            case TowerKind::Mortar:
            case TowerKind::Gravity: return true;
            default:                 return false;
            }
        }
        // One-line description of a type's intrinsic on-hit effect (tooltip).
        std::wstring TowerEffectLabel(unsigned int uImpact)
        {
            if (uImpact & Impact_Slow)      return L"+ Slows enemies hit";
            if (uImpact & Impact_Knockback) return L"+ Knocks enemies back";
            if (uImpact & Impact_Gather)    return L"+ Pulls enemies in";
            if (uImpact & Impact_Burn)      return L"+ Burns enemies hit";
            return L"";
        }
        // Per-type buy tooltip (name + stats + intrinsic effect).
        std::wstring BuildAttackTowerTooltip(int iId)
        {
            const TowerDef* d = TowerDefById(iId);
            const int iPrice = d ? d->iPrice : kTowerPrice;
            const int iHP    = d ? d->iHP    : kTowerHP;
            std::wstring s = (d ? ToW(d->strName) : L"Tower");
            s += L"  $" + std::to_wstring(iPrice) + L"\n";
            s += L"HP " + std::to_wstring(iHP);
            if (d && d->fDefense > 0.f)
                s += L"  Def " + std::to_wstring(static_cast<int>(d->fDefense * 100.f + 0.5f)) + L"%";
            s += L"\n";
            if (d)
            {
                s += L"Atk x" + F1(d->fAttack) + L"  Spd x" + F1(d->fAttackSpeed) + L"\n";
                if (d->fCritChance > 0.f)
                    s += L"Crit " + std::to_wstring(static_cast<int>(d->fCritChance * 100.f + 0.5f)) +
                         L"% x" + F1(d->fCritMult) + L"\n";
                s += L"Range " + F1(d->fRange) + L"\n";
                const std::wstring wEff = TowerEffectLabel(d->uTowerImpact);
                if (!wEff.empty()) s += wEff + L"\n";
            }
            s += L"Auto-fires its equipped weapon";
            return s;
        }
    }

    TowerIntermissionUI::TowerIntermissionUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
        for (int i = 0; i < kBuyRows; ++i)
        {
            m_iBuyIds[i] = -1; m_eBuyKind[i] = BuyKind::Weapon;
            m_bBuyUsed[i] = false; m_bBuyLocked[i] = false;
        }
    }

    bool TowerIntermissionUI::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        using namespace TowerIntermissionUI_detail;
        const Sizes S = ComputeSizes();

        const float fTitleSize  = (std::max)(18.f, S.fTitleH  * 0.55f);
        const float fItemSize   = (std::max)(13.f, S.fItemH   * 0.50f);
        const float fHeaderSize = (std::max)(14.f, S.fHeaderH * 0.70f);
        m_pTitleFont = Engine::FontManager::GetInst()->CreateFont(
            "tower_shop_title", L"Arial", fTitleSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pItemFont = Engine::FontManager::GetInst()->CreateFont(
            "tower_shop_item", L"Arial", fItemSize, DWRITE_FONT_WEIGHT_BOLD);

        // Helper lambdas to build a Text/Button at the running Y cursor.
        auto makeText = [&](const std::string& tag, float fY, float fH,
                            const std::shared_ptr<Engine::Font>& pFont,
                            Engine::Text::HAlign eHA) -> std::shared_ptr<Engine::Text>
        {
            auto pT = CreateComponent<Engine::Text>(tag);
            if (pT)
            {
                pT->SetFont(pFont);
                pT->SetColor(0xFFFFFFFFu);
                pT->SetHAlign(eHA);
                pT->SetVAlign(Engine::Text::VAlign::Center);
                pT->SetRect(S.fLeftX, fY, S.fW, fH);
            }
            return pT;
        };

        float y = S.fTopY;
        m_pTitle = makeText("tower_shop_title_t", y, S.fTitleH, m_pTitleFont, Engine::Text::HAlign::Center);
        y += S.fTitleH + S.fGap;
        m_pInfoText = makeText("tower_shop_info_t", y, S.fInfoH, m_pItemFont, Engine::Text::HAlign::Center);
        y += S.fInfoH + S.fGap;

        // Player stat panel — floats to the LEFT of the buy (weapons) section
        // as a vertical side panel, so it does NOT push the shop column down.
        // Aligned to the top of the buy section (the current y cursor).
        {
            const float fStatsW = S.fW * 0.6f;
            const float fStatsX = (std::max)(S.fGap, S.fLeftX - fStatsW - S.fGap * 3.f);
            const float fStatsH = S.fItemH * 7.f;   // header + six stat lines
            m_pStatsText = CreateComponent<Engine::Text>("tower_shop_stats_t");
            if (m_pStatsText)
            {
                m_pStatsText->SetFont(m_pItemFont);
                m_pStatsText->SetColor(0xC8E0FFFFu);
                m_pStatsText->SetHAlign(Engine::Text::HAlign::Left);
                m_pStatsText->SetVAlign(Engine::Text::VAlign::Top);
                m_pStatsText->SetRect(fStatsX, y, fStatsW, fStatsH);
            }
        }

        // --- Buy Weapons section ---
        m_pBuyHeader = makeText("tower_shop_buy_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pBuyHeader) m_pBuyHeader->SetString(L"Buy (weapons & towers)");
        y += S.fHeaderH + S.fGap;
        for (int i = 0; i < kBuyRows; ++i)
        {
            m_BuyRect[i] = { S.fLeftX, y, S.fW, S.fItemH };   // hover hit-test
            m_pBuyButtons[i] = CreateComponent<Engine::Button>("tower_shop_buy_b" + std::to_string(i));
            if (m_pBuyButtons[i])
            {
                m_pBuyButtons[i]->SetRect(S.fLeftX, y, S.fW, S.fItemH);
                m_pBuyButtons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                const int idx = i;
                m_pBuyButtons[i]->SetOnClick([this, idx]() { OnBuyItem(idx); });
            }
            // Dark outline copies — created BEFORE the main label so they draw
            // underneath it (sibling draw order = creation order). Eight offsets
            // at a thin 1px radius form an even ring (no chunky blob); RebuildList
            // keeps their string synced.
            const float d = (std::max)(1.f, S.fItemH * 0.03f);
            const float offs[kOutlineCopies][2] = {
                {0,-d}, {d,-d}, {d,0}, {d,d}, {0,d}, {-d,d}, {-d,0}, {-d,-d} };
            for (int k = 0; k < kOutlineCopies; ++k)
            {
                auto pO = makeText(
                    "tower_shop_buy_o" + std::to_string(i) + "_" + std::to_string(k),
                    y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);
                if (pO)
                {
                    pO->SetColor(0x000000FFu);   // opaque black outline
                    pO->SetRect(S.fLeftX + offs[k][0], y + offs[k][1], S.fW, S.fItemH);
                }
                m_pBuyTextOutline[i][k] = pO;
            }
            m_pBuyTexts[i] = makeText("tower_shop_buy_t" + std::to_string(i), y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);

            // Pin/lock toggle — a small button just to the RIGHT of the buy row
            // (the panel is centred, so there's free space there). Click toggles
            // m_bBuyLocked[i]; a pinned slot survives rerolls.
            const float fLockW = S.fItemH * 2.2f;
            const float fLockX = S.fLeftX + S.fW + S.fGap * 2.f;
            m_pLockButtons[i] = CreateComponent<Engine::Button>("tower_shop_lock_b" + std::to_string(i));
            if (m_pLockButtons[i])
            {
                m_pLockButtons[i]->SetRect(fLockX, y, fLockW, S.fItemH);
                m_pLockButtons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                const int idx = i;
                m_pLockButtons[i]->SetOnClick([this, idx]() { OnToggleLock(idx); });
            }
            m_pLockTexts[i] = makeText("tower_shop_lock_t" + std::to_string(i), y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);
            if (m_pLockTexts[i]) m_pLockTexts[i]->SetRect(fLockX, y, fLockW, S.fItemH);

            y += S.fItemH + S.fGap;
        }

        // --- Reroll button (re-rolls all unpinned buy slots for a fee) ---
        // Deliberately smaller than a full-width buy row so it doesn't read as a
        // weapon: a short, centred pill (~40% width, ~70% row height).
        {
            const float fRerollW = S.fW * 0.4f;
            const float fRerollH = S.fItemH * 0.7f;
            const float fRerollX = S.fLeftX + (S.fW - fRerollW) * 0.5f;
            m_pRerollButton = CreateComponent<Engine::Button>("tower_shop_reroll_b");
            if (m_pRerollButton)
            {
                m_pRerollButton->SetRect(fRerollX, y, fRerollW, fRerollH);
                m_pRerollButton->SetTexture(EnsureSolidTexture("tower_shop_reroll_bg", 0x315A7A));
                m_pRerollButton->SetOnClick([this]() { OnReroll(); });
            }
            m_pRerollText = makeText("tower_shop_reroll_t", y, fRerollH, m_pItemFont, Engine::Text::HAlign::Center);
            if (m_pRerollText) m_pRerollText->SetRect(fRerollX, y, fRerollW, fRerollH);
            y += fRerollH + S.fGap;
        }

        // --- Equipped weapons (firing slots) — horizontal icon strip ---
        m_pOwnedHeader = makeText("tower_shop_owned_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pOwnedHeader) m_pOwnedHeader->SetString(L"Equipped (fires) - L-click: unequip / R-click: menu");
        y += S.fHeaderH + S.fGap;
        {
            const float fIconGap = S.fGap;
            const float fIconW    = (S.fW - fIconGap * (kOwnedRows - 1)) / kOwnedRows;
            const float fIconH    = S.fItemH;
            for (int i = 0; i < kOwnedRows; ++i)
            {
                const float fX = S.fLeftX + i * (fIconW + fIconGap);
                m_OwnedRect[i] = { fX, y, fIconW, fIconH };
                m_iOwnedIds[i] = -1;
                m_pOwnedIcons[i] = CreateComponent<Engine::Button>("tower_shop_owned_b" + std::to_string(i));
                if (m_pOwnedIcons[i])
                {
                    m_pOwnedIcons[i]->SetRect(fX, y, fIconW, fIconH);
                    m_pOwnedIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                    const int idx = i;
                    // L-click unequips (→ inventory); R-click pops the weapon menu.
                    m_pOwnedIcons[i]->SetOnClick([this, idx]() { OnEquipSlotClick(idx); });
                    m_pOwnedIcons[i]->SetOnRightClick([this, idx]() { OpenWeaponMenu(m_iOwnedIds[idx], m_OwnedRect[idx]); });
                }
            }
            y += fIconH + S.fGap;
        }

        // --- Weapon inventory (idle, unassigned) — horizontal icon strip ---
        m_pInvHeader = makeText("tower_shop_inv_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pInvHeader) m_pInvHeader->SetString(L"Inventory (idle) - L-click: equip / R-click: menu");
        y += S.fHeaderH + S.fGap;
        {
            const float fIconGap = S.fGap;
            const float fIconW    = (S.fW - fIconGap * (kInvRows - 1)) / kInvRows;
            const float fIconH    = S.fItemH;
            for (int i = 0; i < kInvRows; ++i)
            {
                const float fX = S.fLeftX + i * (fIconW + fIconGap);
                m_InvRect[i] = { fX, y, fIconW, fIconH };
                m_iInvIds[i] = -1;
                m_pInvIcons[i] = CreateComponent<Engine::Button>("tower_shop_inv_b" + std::to_string(i));
                if (m_pInvIcons[i])
                {
                    m_pInvIcons[i]->SetRect(fX, y, fIconW, fIconH);
                    m_pInvIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                    const int idx = i;
                    // L-click equips (→ a free firing slot); R-click pops the menu.
                    m_pInvIcons[i]->SetOnClick([this, idx]() { OnInventoryClick(idx); });
                    m_pInvIcons[i]->SetOnRightClick([this, idx]() { OpenWeaponMenu(m_iInvIds[idx], m_InvRect[idx]); });
                }
            }
            y += fIconH + S.fGap;
        }

        // --- Tower Loadout section (drop targets) ---
        m_pTowerHeader = makeText("tower_shop_tower_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pTowerHeader) m_pTowerHeader->SetString(L"Towers (& heal) - L-click cycle / R-click sell (or use a weapon's Equip)");
        y += S.fHeaderH + S.fGap;
        for (int i = 0; i < kTowerRows; ++i)
        {
            m_TowerRect[i] = { S.fLeftX, y, S.fW, S.fItemH };
            m_pTowerButtons[i] = CreateComponent<Engine::Button>("tower_shop_tower_b" + std::to_string(i));
            if (m_pTowerButtons[i])
            {
                m_pTowerButtons[i]->SetRect(S.fLeftX, y, S.fW, S.fItemH);
                m_pTowerButtons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                const int idx = i;
                // Equip armed (weapon menu) → assign that weapon to this tower;
                // otherwise pop the tower action menu (Merge / Weapon / Sell).
                m_pTowerButtons[i]->SetOnClick([this, idx]() {
                    if (m_iEquipArmedWeaponId >= 0) OnCycleTowerWeapon(idx);
                    else                            OpenTowerMenu(idx);
                });
                m_pTowerButtons[i]->SetOnRightClick([this, idx]() { OnSellTower(idx); });
            }
            m_pTowerTexts[i] = makeText("tower_shop_tower_t" + std::to_string(i), y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);
            y += S.fItemH + S.fGap;
        }

        // --- Unplaced Towers (weapon per bought-but-unplaced tower) ---
        // A horizontal icon strip (like Your Weapons). Leftmost = the next
        // tower placed (FIFO). Drop a weapon on a slot or click to cycle.
        m_pReserveHeader = makeText("tower_shop_reserve_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pReserveHeader) m_pReserveHeader->SetString(L"Unplaced Towers (left = placed first) - click to cycle (or use Equip)");
        y += S.fHeaderH + S.fGap;
        {
            const float fIconGap = S.fGap;
            const float fIconW   = (S.fW - fIconGap * (kReserveRows - 1)) / kReserveRows;
            const float fIconH   = S.fItemH;
            for (int i = 0; i < kReserveRows; ++i)
            {
                const float fX = S.fLeftX + i * (fIconW + fIconGap);
                m_ReserveRect[i] = { fX, y, fIconW, fIconH };
                m_pReserveIcons[i] = CreateComponent<Engine::Button>("tower_shop_reserve_b" + std::to_string(i));
                if (m_pReserveIcons[i])
                {
                    m_pReserveIcons[i]->SetRect(fX, y, fIconW, fIconH);
                    m_pReserveIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
                    const int idx = i;
                    m_pReserveIcons[i]->SetOnClick([this, idx]() { OnCycleReserveWeapon(idx); });
                }
            }
            y += fIconH + S.fGap;
        }

        // --- Start ---
        m_pStartButton = CreateComponent<Engine::Button>("tower_shop_start_b");
        if (m_pStartButton)
        {
            m_pStartButton->SetRect(S.fLeftX, y, S.fW, S.fItemH);
            m_pStartButton->SetTexture(EnsureSolidTexture("tower_shop_start_bg", 0x2E7D32));
            m_pStartButton->SetOnClick([this]() { OnStart(); });
        }
        m_pStartText = makeText("tower_shop_start_t", y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);

        // Drag ghost — created LAST so it renders on top of every other row.
        // Hidden until a drag starts; follows the cursor while dragging.
        m_pDragGhost = CreateComponent<Engine::Button>("tower_shop_dragghost");
        if (m_pDragGhost)
        {
            m_pDragGhost->SetRect(0.f, 0.f, S.fItemH, S.fItemH);
            m_pDragGhost->SetTexture(EnsureSolidTexture("tower_shop_blank", 0x303030));
        }

        // Hover tooltip — dark panel + text, created after the ghost so it draws
        // on top. Rect is set per-frame by HandleTooltip; hidden until hover.
        m_pTooltipBg = CreateComponent<Engine::Button>("tower_shop_tip_bg");
        if (m_pTooltipBg)
            m_pTooltipBg->SetTexture(EnsureSolidTexture("tower_shop_tip_bg_tex", 0x12141C));
        m_pTooltipText = CreateComponent<Engine::Text>("tower_shop_tip_t");
        if (m_pTooltipText)
        {
            m_pTooltipText->SetFont(m_pItemFont);
            m_pTooltipText->SetColor(0xEDEFF5FFu);
            m_pTooltipText->SetHAlign(Engine::Text::HAlign::Left);
            m_pTooltipText->SetVAlign(Engine::Text::VAlign::Top);
        }

        // Weapon action menu — a dark panel + three buttons (Sell / Merge /
        // Equip). Created last so it draws above every row. Rects are set per
        // open in OpenWeaponMenu; the OnClick handlers act on m_iMenuWeaponId
        // (current at click time), so they're bound once here.
        m_pMenuBg = CreateComponent<Engine::Button>("tower_shop_menu_bg");
        if (m_pMenuBg)
            m_pMenuBg->SetTexture(EnsureSolidTexture("tower_shop_menu_bg_tex", 0x12141C));
        for (int i = 0; i < kMenuRows; ++i)
        {
            m_pMenuButtons[i] = CreateComponent<Engine::Button>("tower_shop_menu_b" + std::to_string(i));
            if (m_pMenuButtons[i])
                m_pMenuButtons[i]->SetTexture(EnsureSolidTexture("tower_shop_menu_btn", 0x2A2E3A));
            m_pMenuTexts[i] = CreateComponent<Engine::Text>("tower_shop_menu_t" + std::to_string(i));
            if (m_pMenuTexts[i])
            {
                m_pMenuTexts[i]->SetFont(m_pItemFont);
                m_pMenuTexts[i]->SetColor(0xFFFFFFFFu);
                m_pMenuTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pMenuTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
            }
        }
        // The panel is shared between the weapon menu and the tower menu; route
        // each button to the matching handler by which menu is open. Tower-menu
        // rows are Merge / Weapon / Sell (vs the weapon menu's Sell / Merge / Equip).
        if (m_pMenuButtons[0]) m_pMenuButtons[0]->SetOnClick([this]() { if (m_iMenuTowerRow >= 0) OnTowerMenuMerge(); else OnMenuSell();  });
        if (m_pMenuButtons[1]) m_pMenuButtons[1]->SetOnClick([this]() { if (m_iMenuTowerRow >= 0) OnTowerMenuCycle(); else OnMenuMerge(); });
        if (m_pMenuButtons[2]) m_pMenuButtons[2]->SetOnClick([this]() { if (m_iMenuTowerRow >= 0) OnTowerMenuSell();  else OnMenuEquip(); });

        Hide();
        return true;
    }

    void TowerIntermissionUI::Show()
    {
        if (m_pTitle)          m_pTitle->Enable();
        if (m_pInfoText)       m_pInfoText->Enable();
        if (m_pStatsText)      m_pStatsText->Enable();
        if (m_pBuyHeader)      m_pBuyHeader->Enable();
        if (m_pOwnedHeader)    m_pOwnedHeader->Enable();
        if (m_pInvHeader)      m_pInvHeader->Enable();
        if (m_pTowerHeader)    m_pTowerHeader->Enable();
        if (m_pReserveHeader)  m_pReserveHeader->Enable();
        if (m_pStartButton)    m_pStartButton->Enable();
        if (m_pStartText)      m_pStartText->Enable();
        if (m_pRerollButton)   m_pRerollButton->Enable();
        if (m_pRerollText)     m_pRerollText->Enable();
        // Buy / owned / tower rows are enabled selectively by RebuildList.
        // The drag ghost stays hidden until a drag begins.
    }

    void TowerIntermissionUI::Hide()
    {
        if (m_pTitle)          m_pTitle->Disable();
        if (m_pInfoText)       m_pInfoText->Disable();
        if (m_pStatsText)      m_pStatsText->Disable();
        if (m_pBuyHeader)      m_pBuyHeader->Disable();
        if (m_pOwnedHeader)    m_pOwnedHeader->Disable();
        if (m_pInvHeader)      m_pInvHeader->Disable();
        if (m_pTowerHeader)    m_pTowerHeader->Disable();
        if (m_pReserveHeader)  m_pReserveHeader->Disable();
        if (m_pStartButton)    m_pStartButton->Disable();
        if (m_pStartText)      m_pStartText->Disable();
        if (m_pRerollButton)   m_pRerollButton->Disable();
        if (m_pRerollText)     m_pRerollText->Disable();
        if (m_pDragGhost)      m_pDragGhost->Disable();
        if (m_pTooltipBg)      m_pTooltipBg->Disable();
        if (m_pTooltipText)    m_pTooltipText->Disable();
        CloseWeaponMenu();
        m_iEquipArmedWeaponId  = -1;
        m_bEquipArmedThisFrame = false;
        for (int i = 0; i < kOwnedRows; ++i)
            if (m_pOwnedIcons[i]) m_pOwnedIcons[i]->Disable();
        for (int i = 0; i < kInvRows; ++i)
            if (m_pInvIcons[i]) m_pInvIcons[i]->Disable();
        for (int i = 0; i < kBuyRows; ++i)
        {
            if (m_pBuyButtons[i]) m_pBuyButtons[i]->Disable();
            if (m_pBuyTexts[i])   m_pBuyTexts[i]->Disable();
            if (m_pLockButtons[i]) m_pLockButtons[i]->Disable();
            if (m_pLockTexts[i])   m_pLockTexts[i]->Disable();
            for (int k = 0; k < kOutlineCopies; ++k)
                if (m_pBuyTextOutline[i][k]) m_pBuyTextOutline[i][k]->Disable();
        }
        for (int i = 0; i < kTowerRows; ++i)
        {
            if (m_pTowerButtons[i]) m_pTowerButtons[i]->Disable();
            if (m_pTowerTexts[i])   m_pTowerTexts[i]->Disable();
        }
        for (int i = 0; i < kReserveRows; ++i)
            if (m_pReserveIcons[i]) m_pReserveIcons[i]->Disable();
    }

    void TowerIntermissionUI::RollCatalog()
    {
        // Random subset of the shop pool (Fisher-Yates partial shuffle,
        // std::rand to match the rest of the game). The pool is the v2 weapon
        // catalogue ONLY — crafted weapons (from crafted.csv) are excluded so
        // they never roll into the round shop. Owned weapons can still appear
        // (buying adds another copy, shown with a "xN" owned count); the equip
        // section is where you pick among them.
        // The catalog mixes weapons and towers: every shop-available weapon plus
        // an attack-tower and a heal-tower entry, shuffled, first kBuyRows shown.
        // So towers roll in randomly alongside weapons (same as how a weapon may
        // or may not appear on any given shop open).
        struct Cand { BuyKind kind; int id; };
        std::vector<Cand> all;
        const int iRound = m_fnRound ? m_fnRound() : 0;   // gate by the upcoming round
        const std::vector<int> vecCrafted = WeaponDatabase::GetInst().AllCraftedLiveIds();
        for (int id : WeaponDatabase::GetInst().ShopWeaponIds(iRound))
        {
            if (std::find(vecCrafted.begin(), vecCrafted.end(), id) != vecCrafted.end())
                continue;   // skip session-crafted weapons
            if (IsWeaponHeldByTower(id)) continue;   // locked to a tower — not buyable
            all.push_back({ BuyKind::Weapon, id });
        }
        // One entry per round-available attack tower TYPE (Gatling / Frost /
        // Mortar / Gravity), so types roll into the shop alongside weapons.
        {
            const int gateRound = iRound < 1 ? 1 : iRound;
            for (const TowerDef& d : TowerDatabase::GetInst().All())
            {
                if (!TowerIntermissionUI_detail::IsBuyableAttackType(d)) continue;
                if (d.iFirstRound > gateRound) continue;
                if (d.iLastRound != 0 && gateRound > d.iLastRound) continue;
                all.push_back({ BuyKind::Tower, d.iId });
            }
        }
        all.push_back({ BuyKind::HealTower, -1 });

        // Pinned slots keep their current item (and are excluded from the fresh
        // roll so a rolled slot never duplicates a pin). Build the roll pool from
        // the candidates not already pinned, then shuffle it.
        auto bPinned = [&](const Cand& c) -> bool
        {
            for (int i = 0; i < kBuyRows; ++i)
                if (m_bBuyLocked[i] && m_bBuyUsed[i] &&
                    m_eBuyKind[i] == c.kind && m_iBuyIds[i] == c.id)
                    return true;
            return false;
        };
        std::vector<Cand> pool;
        for (const Cand& c : all)
            if (!bPinned(c)) pool.push_back(c);

        const int n = static_cast<int>(pool.size());
        for (int i = 0; i < n; ++i)   // full Fisher-Yates shuffle of the pool
        {
            const int j = i + std::rand() % (n - i);
            std::swap(pool[i], pool[j]);
        }
        int p = 0;
        for (int i = 0; i < kBuyRows; ++i)
        {
            if (m_bBuyLocked[i] && m_bBuyUsed[i]) continue;   // keep the pinned item
            if (p < n) { m_eBuyKind[i] = pool[p].kind; m_iBuyIds[i] = pool[p].id; m_bBuyUsed[i] = true; ++p; }
            else        { m_eBuyKind[i] = BuyKind::Weapon; m_iBuyIds[i] = -1; m_bBuyUsed[i] = false; }
        }
    }

    void TowerIntermissionUI::RerollBuySlot(int iIndex)
    {
        if (iIndex < 0 || iIndex >= kBuyRows) return;
        if (m_bBuyLocked[iIndex]) return;   // pinned slot keeps its item

        // Same candidate pool RollCatalog draws from: shop weapons (minus
        // session-crafted) plus an attack-tower and a heal-tower entry.
        struct Cand { BuyKind kind; int id; };
        std::vector<Cand> all;
        const int iRound = m_fnRound ? m_fnRound() : 0;   // gate by the upcoming round
        const std::vector<int> vecCrafted = WeaponDatabase::GetInst().AllCraftedLiveIds();
        for (int id : WeaponDatabase::GetInst().ShopWeaponIds(iRound))
        {
            if (std::find(vecCrafted.begin(), vecCrafted.end(), id) != vecCrafted.end())
                continue;
            if (IsWeaponHeldByTower(id)) continue;   // locked to a tower — not buyable
            all.push_back({ BuyKind::Weapon, id });
        }
        // One entry per round-available attack tower TYPE (Gatling / Frost /
        // Mortar / Gravity), so types roll into the shop alongside weapons.
        {
            const int gateRound = iRound < 1 ? 1 : iRound;
            for (const TowerDef& d : TowerDatabase::GetInst().All())
            {
                if (!TowerIntermissionUI_detail::IsBuyableAttackType(d)) continue;
                if (d.iFirstRound > gateRound) continue;
                if (d.iLastRound != 0 && gateRound > d.iLastRound) continue;
                all.push_back({ BuyKind::Tower, d.iId });
            }
        }
        all.push_back({ BuyKind::HealTower, -1 });

        // Prefer a pick that isn't already displayed in another slot, so the
        // replacement is a genuinely different item. Fall back to the full pool
        // if every candidate is already on screen (small pool).
        auto bShownElsewhere = [&](const Cand& c) -> bool
        {
            for (int i = 0; i < kBuyRows; ++i)
            {
                if (i == iIndex || !m_bBuyUsed[i]) continue;
                if (m_eBuyKind[i] == c.kind && m_iBuyIds[i] == c.id) return true;
            }
            return false;
        };
        std::vector<Cand> fresh;
        for (const Cand& c : all)
            if (!bShownElsewhere(c)) fresh.push_back(c);

        const std::vector<Cand>& pick = fresh.empty() ? all : fresh;
        if (pick.empty()) return;
        const Cand& c = pick[std::rand() % pick.size()];
        m_eBuyKind[iIndex] = c.kind;
        m_iBuyIds[iIndex]  = c.id;
        m_bBuyUsed[iIndex] = true;
    }

    int TowerIntermissionUI::WeaponPriceOf(int iWeaponId) const
    {
        const WeaponDef* pDef = WeaponDatabase::GetInst().Get(iWeaponId);
        if (pDef && pDef->iPrice > 0) return pDef->iPrice;
        return kWeaponPrice;   // 0/unset => global default
    }

    bool TowerIntermissionUI::IsWeaponHeldByTower(int iWeaponId) const
    {
        if (iWeaponId < 0) return false;
        // Placed attack towers in the scene.
        auto* pOwnerT = GetGameObjectOwner();
        Engine::Scene* pSceneT = pOwnerT ? pOwnerT->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayerT =
            pSceneT ? pSceneT->FindLayer(DEFAULT_LAYER) : nullptr;
        if (pLayerT)
            for (const auto& p : pLayerT->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
                if (std::static_pointer_cast<Tower>(p)->GetWeaponId() == iWeaponId) return true;
            }
        // Bought-but-unplaced (reserve) towers.
        auto& tm = TowerManager::GetInst();
        const int n = tm.ReserveCount();
        for (int i = 0; i < n; ++i)
            if (tm.ReserveWeaponRaw(i) == iWeaponId) return true;
        return false;
    }

    bool TowerIntermissionUI::IsWeaponHeldByOtherTower(
        int iWeaponId, const Engine::GameObject* pExcludeTower, int iExcludeReserve) const
    {
        if (iWeaponId < 0) return false;
        auto* pOwnerO = GetGameObjectOwner();
        Engine::Scene* pSceneO = pOwnerO ? pOwnerO->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayerO =
            pSceneO ? pSceneO->FindLayer(DEFAULT_LAYER) : nullptr;
        if (pLayerO)
            for (const auto& p : pLayerO->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
                if (p.get() == pExcludeTower) continue;   // the tower being assigned
                if (std::static_pointer_cast<Tower>(p)->GetWeaponId() == iWeaponId) return true;
            }
        auto& tm = TowerManager::GetInst();
        const int n = tm.ReserveCount();
        for (int i = 0; i < n; ++i)
        {
            if (i == iExcludeReserve) continue;
            if (tm.ReserveWeaponRaw(i) == iWeaponId) return true;
        }
        return false;
    }

    int TowerIntermissionUI::PlacedTowerCount() const
    {
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (!pLayer) return 0;
        int iCount = 0;
        for (const auto& p : pLayer->GetGameObjectList())
            if (p && p->IsActive() && p->GetTag() == "Tower") ++iCount;
        return iCount;
    }

    void TowerIntermissionUI::RebuildList()
    {
        using namespace TowerIntermissionUI_detail;

        auto pPlayer = m_pTarget.lock();
        const int iMoney   = Wallet::GetInst().Money();
        const bool bLoadoutFull =
            pPlayer && pPlayer->GetWeaponSlotCount() >= Player::GetMaxWeaponSlots();

        if (m_pTitle)
        {
            const int iRound = m_fnRound ? m_fnRound() : 0;
            m_pTitle->SetString(L"Round " + std::to_wstring(iRound) + L" Shop");
        }
        if (m_pInfoText)
        {
            const int iOwned  = TowerManager::GetInst().TowersOwned();
            const int iHeal   = TowerManager::GetInst().HealTowersOwned();
            m_pInfoText->SetString(
                L"Gold: " + std::to_wstring(iMoney) +
                L"    Towers: " + std::to_wstring(iOwned) +
                L"    Heal: " + std::to_wstring(iHeal));
        }

        // --- Player stat panel (two lines) ---
        if (m_pStatsText)
        {
            // Rounded percent ("130%") and one-decimal ("5.6") formatters —
            // no <sstream>/locale, integer math only.
            auto pct  = [](float v) { return std::to_wstring(static_cast<int>(v * 100.f + 0.5f)) + L"%"; };
            auto fmt1 = [](float v)
            {
                const int t = static_cast<int>(v * 10.f + 0.5f);
                return std::to_wstring(t / 10) + L"." + std::to_wstring(t % 10);
            };
            std::wstring wStats;
            if (pPlayer)
            {
                wStats =
                    L"[ Stats ]\n"
                    L"Lv. " + std::to_wstring(pPlayer->GetLevel()) +
                    L"\nHP  " + std::to_wstring(pPlayer->GetHP()) +
                    L"/" + std::to_wstring(pPlayer->GetMaxHP()) +
                    L"\nATK " + pct(pPlayer->GetDamageMult()) +
                    L"\nSPD " + fmt1(pPlayer->GetMoveSpeed()) +
                    L"\nCRIT " + pct(pPlayer->GetCritChance()) +
                    L"\nDEF " + pct(pPlayer->GetDamageReduction());
            }
            m_pStatsText->SetString(wStats);
        }

        // Keep a buy row's dark outline copies in sync with its label: same
        // string when shown (they stay black), hidden otherwise.
        auto syncOutline = [&](int i, const std::wstring& str, bool bShow)
        {
            for (int k = 0; k < kOutlineCopies; ++k)
            {
                if (!m_pBuyTextOutline[i][k]) continue;
                if (bShow) { m_pBuyTextOutline[i][k]->SetString(str); m_pBuyTextOutline[i][k]->Enable(); }
                else        m_pBuyTextOutline[i][k]->Disable();
            }
        };

        // Refresh a row's pin button: gold "Pinned" when locked, dim "Pin"
        // otherwise; hidden when the slot is empty.
        auto refreshLock = [&](int i, bool bShow)
        {
            const bool bLocked = m_bBuyLocked[i];
            if (m_pLockButtons[i])
            {
                if (!bShow) { m_pLockButtons[i]->Disable(); }
                else
                {
                    m_pLockButtons[i]->SetTexture(EnsureSolidTexture(
                        bLocked ? "tower_shop_lock_on" : "tower_shop_lock_off",
                        bLocked ? 0xC9A227 : 0x303030));
                    m_pLockButtons[i]->Enable();
                }
            }
            if (m_pLockTexts[i])
            {
                if (!bShow) { m_pLockTexts[i]->Disable(); }
                else
                {
                    m_pLockTexts[i]->SetColor(bLocked ? 0x202020FFu : 0xC0C0C0FFu);
                    m_pLockTexts[i]->SetString(bLocked ? L"Pinned" : L"Pin");
                    m_pLockTexts[i]->Enable();
                }
            }
        };

        // --- Buy catalog rows (random weapons + towers) ---
        for (int i = 0; i < kBuyRows; ++i)
        {
            const bool bHas = m_bBuyUsed[i];
            const int  id   = m_iBuyIds[i];
            if (!bHas)
            {
                if (m_pBuyButtons[i]) m_pBuyButtons[i]->Disable();
                if (m_pBuyTexts[i])   m_pBuyTexts[i]->Disable();
                syncOutline(i, L"", false);
                refreshLock(i, false);
                continue;
            }
            refreshLock(i, true);
            // Tower rows: a flat colour button + price (blue=attack, green=heal).
            if (m_eBuyKind[i] == BuyKind::Tower || m_eBuyKind[i] == BuyKind::HealTower)
            {
                const bool bHeal  = (m_eBuyKind[i] == BuyKind::HealTower);
                const TowerDef* pTowerDef = bHeal ? nullptr : TowerIntermissionUI_detail::TowerDefById(id);
                const int  iPrice = bHeal
                    ? TowerIntermissionUI_detail::TowerBuyPrice(true)
                    : (pTowerDef ? pTowerDef->iPrice : kTowerPrice);
                const std::wstring wTowerName = bHeal
                    ? std::wstring(L"Heal Tower")
                    : (pTowerDef ? ToW(pTowerDef->strName) : std::wstring(L"Tower"));
                // All towers (attack + heal) share one kMaxTowers cap; once the
                // combined owned count reaches it, every tower row greys out and
                // shows (MAX) instead of the price (the buy handler enforces the
                // same combined guard).
                const bool bTowerCapped =
                    TowerManager::GetInst().TowersOwned()
                    + TowerManager::GetInst().HealTowersOwned() >= kMaxTowers;
                if (m_pBuyButtons[i])
                {
                    m_pBuyButtons[i]->SetTexture(EnsureSolidTexture(
                        bHeal ? "tower_shop_buyheal_bg" : "tower_shop_buytower_bg",
                        bHeal ? 0x1E7A3C : 0x18558A));
                    m_pBuyButtons[i]->Enable();
                }
                const std::wstring wTowerLabel = bTowerCapped
                    ? (wTowerName + L"  (MAX)")
                    : (wTowerName + L"  $" + std::to_wstring(iPrice));
                if (m_pBuyTexts[i])
                {
                    m_pBuyTexts[i]->SetColor(
                        (!bTowerCapped && iMoney >= iPrice) ? 0xFFFFFFFFu : 0x808080FFu);
                    m_pBuyTexts[i]->SetString(wTowerLabel);
                    m_pBuyTexts[i]->Enable();
                }
                syncOutline(i, wTowerLabel, true);
                continue;
            }
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const std::wstring wName = pDef ? ToW(pDef->strName) : L"Weapon";
            const int  iOwnedCopies = pPlayer ? pPlayer->CountOwnedWeapon(id) : 0;

            if (m_pBuyButtons[i])
            {
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pBuyButtons[i]->SetTexture(
                    EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
                m_pBuyButtons[i]->Enable();
            }
            if (m_pBuyTexts[i])
            {
                const int iPrice = WeaponPriceOf(id);   // per-weapon price
                std::wstring wLabel;
                unsigned int uTextColor;
                if (bLoadoutFull)
                {
                    // Every buy now takes a loadout slot (duplicate copies are
                    // mergeable in the weapon menu), so a full loadout blocks it.
                    wLabel = wName + L"  (FULL)";
                    uTextColor = 0x808080FFu;
                }
                else
                {
                    // A "x2"-style suffix flags how many copies are already owned
                    // so the player can plan a merge.
                    const std::wstring wOwned = iOwnedCopies > 0
                        ? L"  x" + std::to_wstring(iOwnedCopies) : std::wstring();
                    wLabel = wName + wOwned + L"  $" + std::to_wstring(iPrice);
                    uTextColor = (iMoney >= iPrice) ? 0x60FF60FFu : 0x808080FFu;
                }
                m_pBuyTexts[i]->SetColor(uTextColor);
                m_pBuyTexts[i]->SetString(wLabel);
                m_pBuyTexts[i]->Enable();
                syncOutline(i, wLabel, true);
            }
        }

        // --- Reroll cost label (white when affordable, grey otherwise) ---
        if (m_pRerollText)
        {
            const int iCost = RerollCost();
            m_pRerollText->SetColor(iMoney >= iCost ? 0xFFFFFFFFu : 0x808080FFu);
            m_pRerollText->SetString(L"Reroll  $" + std::to_wstring(iCost));
        }

        // --- Equipped weapons (firing slots) ---
        // Show the player's equipped weapons, then a dim [+] for each remaining
        // free firing slot (up to kMaxEquipSlots), then disable the rest.
        const std::vector<int> vecEquipped =
            pPlayer ? pPlayer->GetEquippedWeaponIds() : std::vector<int>{};
        const int iEquipCap = Player::GetMaxEquipSlots();
        for (int i = 0; i < kOwnedRows; ++i)
        {
            if (!m_pOwnedIcons[i]) continue;
            const bool bHas = i < static_cast<int>(vecEquipped.size());
            if (bHas)
            {
                const int id = vecEquipped[i];
                m_iOwnedIds[i] = id;
                const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pOwnedIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
                m_pOwnedIcons[i]->Enable();
            }
            else if (i < iEquipCap)
            {
                // Empty firing slot — dim [+] placeholder (drop / equip target).
                m_iOwnedIds[i] = -1;
                m_pOwnedIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_equip_empty", 0x242424));
                m_pOwnedIcons[i]->Enable();
            }
            else
            {
                m_iOwnedIds[i] = -1;
                m_pOwnedIcons[i]->Disable();
            }
        }

        // --- Weapon inventory (idle) ---
        const std::vector<int> vecInv =
            pPlayer ? pPlayer->GetInventoryWeaponIds() : std::vector<int>{};
        for (int i = 0; i < kInvRows; ++i)
        {
            if (!m_pInvIcons[i]) continue;
            const bool bHas = i < static_cast<int>(vecInv.size());
            const int  id   = bHas ? vecInv[i] : -1;
            m_iInvIds[i] = id;
            if (!bHas) { m_pInvIcons[i]->Disable(); continue; }
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
            m_pInvIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
            m_pInvIcons[i]->Enable();
        }

        // --- Tower Loadout rows (each placed tower) ---
        // Gather the placed towers from the scene (stable while frozen). Both
        // attack ("Tower") and heal ("HealTower") towers go in one list so both
        // can be sold here; bHeal marks which (heal rows can't cycle a weapon).
        struct RowEnt { std::shared_ptr<Engine::GameObject> obj; bool bHeal; };
        std::vector<RowEnt> vecTowers;
        {
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pLayer)
                for (const auto& p : pLayer->GetGameObjectList())
                {
                    if (!p || !p->IsActive()) continue;
                    if      (p->GetTag() == "Tower")     vecTowers.push_back({ p, false });
                    else if (p->GetTag() == "HealTower") vecTowers.push_back({ p, true  });
                }
        }
        m_iTowerCount = (std::min)(kTowerRows, static_cast<int>(vecTowers.size()));
        for (int i = 0; i < kTowerRows; ++i)
        {
            const bool bHas = i < m_iTowerCount;
            if (!bHas)
            {
                m_pTowerRowRefs[i].reset();
                m_bTowerRowIsHeal[i] = false;
                if (m_pTowerButtons[i]) m_pTowerButtons[i]->Disable();
                if (m_pTowerTexts[i])   m_pTowerTexts[i]->Disable();
                continue;
            }
            m_pTowerRowRefs[i]   = vecTowers[i].obj;
            m_bTowerRowIsHeal[i] = vecTowers[i].bHeal;

            if (vecTowers[i].bHeal)
            {
                // Heal tower: no weapon to cycle — a flat green row, R-click sells.
                if (m_pTowerButtons[i])
                {
                    m_pTowerButtons[i]->SetTexture(
                        EnsureSolidTexture("tower_shop_healrow", 0x1E7A3C));
                    m_pTowerButtons[i]->Enable();
                }
                if (m_pTowerTexts[i])
                {
                    m_pTowerTexts[i]->SetColor(0x80FF80FFu);
                    m_pTowerTexts[i]->SetString(L"Heal Tower " + std::to_wstring(i + 1));
                    m_pTowerTexts[i]->Enable();
                }
                continue;
            }

            auto pT = std::static_pointer_cast<Tower>(vecTowers[i].obj);
            const int wid = pT->GetWeaponId();
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(wid);
            const std::wstring wName = pDef ? ToW(pDef->strName) : L"None";

            if (m_pTowerButtons[i])
            {
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pTowerButtons[i]->SetTexture(
                    EnsureSolidTexture("tower_shop_w_" + std::to_string(wid), uColor));
                m_pTowerButtons[i]->Enable();
            }
            if (m_pTowerTexts[i])
            {
                m_pTowerTexts[i]->SetColor(0xFFFFFFFFu);
                m_pTowerTexts[i]->SetString(L"Tower " + std::to_wstring(i + 1) +
                    L" (Lv." + std::to_wstring(pT->GetLevel()) + L"): " + wName);
                m_pTowerTexts[i]->Enable();
            }
        }

        // --- Unplaced Towers (reserve weapon per bought-but-unplaced tower) ---
        // Each icon's colour is the weapon that reserve tower will fire. A raw
        // -1 (unconfigured = freshly bought) shows a dim "no weapon" slot — the
        // tower can't be PLACED until a weapon is equipped here (click to cycle /
        // drag one on), so it must read as empty rather than borrowing a colour.
        const int iReserve  = TowerManager::GetInst().ReserveCount();
        m_iReserveCount = (std::min)(kReserveRows, iReserve);
        for (int i = 0; i < kReserveRows; ++i)
        {
            if (!m_pReserveIcons[i]) continue;
            if (i >= m_iReserveCount) { m_pReserveIcons[i]->Disable(); continue; }
            const int wid = TowerManager::GetInst().ReserveWeaponRaw(i);
            if (wid < 0)
            {
                // Weaponless — dim slot, not placeable yet.
                m_pReserveIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_reserve_empty", 0x242424));
            }
            else
            {
                const WeaponDef* pDef = WeaponDatabase::GetInst().Get(wid);
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pReserveIcons[i]->SetTexture(
                    EnsureSolidTexture("tower_shop_w_" + std::to_string(wid), uColor));
            }
            m_pReserveIcons[i]->Enable();
        }

        // --- Start ---
        if (m_pStartText)
        {
            const int iNext = m_fnRound ? m_fnRound() : 0;
            m_pStartText->SetString(L"Start Round " + std::to_wstring(iNext));
        }
    }

    void TowerIntermissionUI::OnBuyItem(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        if (iIndex < 0 || iIndex >= kBuyRows || !m_bBuyUsed[iIndex]) return;

        // Tower catalog rows: buy adds to the tower inventory (a new unplaced
        // tower; its weapon defaults to the current one, configurable in the
        // Unplaced Towers strip).
        // Combined cap: attack + heal towers share one kMaxTowers budget.
        const int iTowersTotal = TowerManager::GetInst().TowersOwned()
                               + TowerManager::GetInst().HealTowersOwned();
        if (m_eBuyKind[iIndex] == BuyKind::Tower)
        {
            if (iTowersTotal >= kMaxTowers) return;   // tower cap reached
            const int       iTowerId  = m_iBuyIds[iIndex];
            const TowerDef* pTowerDef = TowerIntermissionUI_detail::TowerDefById(iTowerId);
            const int       iPrice    = pTowerDef ? pTowerDef->iPrice : kTowerPrice;
            if (!Wallet::GetInst().TrySpend(iPrice)) return;
            TowerManager::GetInst().AddTower(iTowerId);
            m_bBuyLocked[iIndex] = false;   // buying consumes a pinned item → reroll
            RerollBuySlot(iIndex);
            RebuildList();
            return;
        }
        if (m_eBuyKind[iIndex] == BuyKind::HealTower)
        {
            if (iTowersTotal >= kMaxTowers) return;   // shared tower cap reached
            if (!Wallet::GetInst().TrySpend(TowerIntermissionUI_detail::TowerBuyPrice(true))) return;
            TowerManager::GetInst().AddHealTower();
            m_bBuyLocked[iIndex] = false;   // buying consumes a pinned item → reroll
            RerollBuySlot(iIndex);
            RebuildList();
            return;
        }

        const int id = m_iBuyIds[iIndex];
        if (id < 0) return;

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        // Every purchase adds a fresh copy as its own loadout slot (duplicate
        // copies are combined later via the weapon menu's Merge), so a full
        // loadout blocks any buy — owned or not.
        if (pPlayer->GetWeaponSlotCount() >= Player::GetMaxWeaponSlots()) return;   // loadout full
        if (!Wallet::GetInst().TrySpend(WeaponPriceOf(id))) return;   // can't afford

        // Add the copy to the player's loadout and make it the default weapon
        // for newly placed towers (assign it to specific towers in the Tower
        // Loadout section below).
        pPlayer->AddWeaponCopy(id);
        TowerManager::GetInst().SetCurrentWeaponId(id);
        m_bBuyLocked[iIndex] = false;   // buying consumes a pinned item → reroll
        RerollBuySlot(iIndex);
        RebuildList();
    }

    void TowerIntermissionUI::OnToggleLock(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        if (iIndex < 0 || iIndex >= kBuyRows || !m_bBuyUsed[iIndex]) return;
        m_bBuyLocked[iIndex] = !m_bBuyLocked[iIndex];
        RebuildList();
    }

    void TowerIntermissionUI::OnSellWeapon(int iWeaponId)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (iWeaponId < 0) return;
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        const int iLevel = pPlayer->GetOwnedWeaponLevel(iWeaponId);
        if (iLevel <= 0) return;   // not owned

        // Refund half of what was invested (the weapon's price was paid once per
        // level via buy + merges), rounded down, at least 1 gold.
        int iRefund = WeaponPriceOf(iWeaponId) * iLevel / 2;
        if (iRefund < 1) iRefund = 1;

        pPlayer->RemoveWeapon(iWeaponId);
        Wallet::GetInst().Add(iRefund);
        RebuildList();
    }

    void TowerIntermissionUI::OnEquipSlotClick(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;
        if (iIndex < 0 || iIndex >= kOwnedRows) return;
        const int id = m_iOwnedIds[iIndex];
        if (id < 0) return;   // empty firing slot — nothing to unequip
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        pPlayer->UnequipWeapon(id);   // move to the inventory
        RebuildList();
    }

    void TowerIntermissionUI::OnInventoryClick(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;
        if (iIndex < 0 || iIndex >= kInvRows) return;
        const int id = m_iInvIds[iIndex];
        if (id < 0) return;
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        pPlayer->EquipWeapon(id);     // move to a free firing slot (no-op if full)
        RebuildList();
    }

    bool TowerIntermissionUI::PointerInOpenMenu() const
    {
        if (m_iMenuWeaponId < 0 && m_iMenuTowerRow < 0) return false;
        auto* pInput = Engine::CInput::GetInst();
        return InRect(static_cast<float>(pInput->GetMouseX()),
                      static_cast<float>(pInput->GetMouseY()), m_MenuPanelRect);
    }

    void TowerIntermissionUI::OpenWeaponMenu(int iWeaponId, const Rect& anchor)
    {
        using namespace TowerIntermissionUI_detail;
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click landed on the already-open menu
        const int id = iWeaponId;
        if (id < 0) return;

        // Opening a menu cancels any pending equip-arm.
        m_iEquipArmedWeaponId = -1;
        if (m_pDragGhost) m_pDragGhost->Disable();
        m_iMenuWeaponId = id;
        m_iMenuTowerRow = -1;   // weapon menu, not tower menu

        // Sell refund mirrors OnSellWeapon (half of kWeaponPrice per level, min
        // 1); merge needs at least two owned copies and greys out otherwise.
        auto pPlayer = m_pTarget.lock();
        int iLevel = pPlayer ? pPlayer->GetOwnedWeaponLevel(id) : 1;
        if (iLevel < 1) iLevel = 1;
        const int iPrice = WeaponPriceOf(id);   // per-weapon price
        int iRefund = iPrice * iLevel / 2;
        if (iRefund < 1) iRefund = 1;
        const int  iCopies    = pPlayer ? pPlayer->CountOwnedWeapon(id) : 0;
        const bool bCanMerge  = iCopies >= 2;

        // Lay the panel out just under the clicked icon, flipping above / clamping
        // to the window if it would spill off-screen.
        const Sizes S = ComputeSizes();
        const float Wpx = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float Hpx = static_cast<float>(Engine::Window::GetInst()->GetHeight());
        const float fRowH = S.fItemH;
        const float fW    = (std::max)(150.f, S.fW * 0.34f);
        const float fH    = fRowH * kMenuRows;
        float px = anchor.x;
        float py = anchor.y + anchor.h + S.fGap;
        if (px + fW > Wpx) px = Wpx - fW;
        if (px < 0.f) px = 0.f;
        if (py + fH > Hpx) py = anchor.y - fH - S.fGap;   // flip above
        if (py < 0.f) py = 0.f;
        m_MenuPanelRect = { px, py, fW, fH };

        if (m_pMenuBg) { m_pMenuBg->SetRect(px, py, fW, fH); m_pMenuBg->Enable(); }
        for (int i = 0; i < kMenuRows; ++i)
        {
            const float ry = py + fRowH * i;
            if (m_pMenuButtons[i]) { m_pMenuButtons[i]->SetRect(px, ry, fW, fRowH); m_pMenuButtons[i]->Enable(); }
            if (m_pMenuTexts[i])   { m_pMenuTexts[i]->SetRect(px, ry, fW, fRowH);   m_pMenuTexts[i]->Enable(); }
        }
        if (m_pMenuTexts[0])
        {
            m_pMenuTexts[0]->SetString(L"Sell  +" + std::to_wstring(iRefund) + L" G");
            m_pMenuTexts[0]->SetColor(0xFFD000FFu);   // gold
        }
        if (m_pMenuTexts[1])
        {
            // Merge two owned copies into one higher-level copy (free). Greyed
            // until the player owns at least two; the count hints at progress.
            m_pMenuTexts[1]->SetString(L"Merge  (x" + std::to_wstring(iCopies) + L")");
            m_pMenuTexts[1]->SetColor(bCanMerge ? 0x60FF60FFu : 0x808080FFu);
        }
        if (m_pMenuTexts[2])
        {
            m_pMenuTexts[2]->SetString(L"Equip on tower");
            m_pMenuTexts[2]->SetColor(0xFFFFFFFFu);
        }
    }

    void TowerIntermissionUI::CloseWeaponMenu()
    {
        m_iMenuWeaponId = -1;
        m_iMenuTowerRow = -1;
        if (m_pMenuBg) m_pMenuBg->Disable();
        for (int i = 0; i < kMenuRows; ++i)
        {
            if (m_pMenuButtons[i]) m_pMenuButtons[i]->Disable();
            if (m_pMenuTexts[i])   m_pMenuTexts[i]->Disable();
        }
    }

    void TowerIntermissionUI::OnMenuSell()
    {
        const int id = m_iMenuWeaponId;
        CloseWeaponMenu();
        if (id >= 0) OnSellWeapon(id);   // refunds gold + RebuildList
    }

    void TowerIntermissionUI::OnMenuMerge()
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        const int id = m_iMenuWeaponId;
        CloseWeaponMenu();
        if (id < 0) return;
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        // Combine two owned copies of this weapon into one higher-level copy,
        // freeing a slot (handles evolution at the threshold). Free — the cost
        // was the second copy. No-op when fewer than two copies are owned.
        if (pPlayer->MergeWeapon(id)) RebuildList();
    }

    void TowerIntermissionUI::OnMenuEquip()
    {
        const int id = m_iMenuWeaponId;
        CloseWeaponMenu();
        if (id < 0) return;
        // Arm: the next tower-row / reserve-slot click equips this weapon there
        // (HandleWeaponMenu drives the ghost + cancel; the row buttons' OnClick
        // do the actual assignment).
        m_iEquipArmedWeaponId   = id;
        m_bEquipArmedThisFrame  = true;   // skip this click in HandleWeaponMenu
        if (m_pDragGhost)
        {
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const unsigned int uColor = pDef ? pDef->uColorRGB : 0x808080;
            m_pDragGhost->SetTexture(TowerIntermissionUI_detail::EnsureSolidTexture(
                "tower_shop_w_" + std::to_string(id), uColor));
            m_pDragGhost->Enable();
        }
    }

    // Placed attack towers of a given TYPE (towers.csv def id). Merging groups
    // by type, NOT weapon: any two same-type towers combine regardless of the
    // weapons they have equipped.
    std::vector<std::shared_ptr<Tower>> TowerIntermissionUI::CollectAttackTowers(int iTowerDefId) const
    {
        std::vector<std::shared_ptr<Tower>> vec;
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (!pLayer) return vec;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
            auto pT = std::static_pointer_cast<Tower>(p);
            if (pT->GetTowerDefId() == iTowerDefId) vec.push_back(pT);
        }
        return vec;
    }

    void TowerIntermissionUI::OpenTowerMenu(int iRow)
    {
        using namespace TowerIntermissionUI_detail;
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click landed on the already-open menu
        if (iRow < 0 || iRow >= m_iTowerCount) return;

        // Opening a menu cancels any pending equip-arm and the weapon menu.
        m_iEquipArmedWeaponId = -1;
        if (m_pDragGhost) m_pDragGhost->Disable();
        m_iMenuWeaponId = -1;
        m_iMenuTowerRow = iRow;

        // Merge needs >= 2 placed towers of the same TYPE (regardless of weapon)
        // and a kept copy below the cap; heal towers have no level and never merge.
        const bool bHeal = m_bTowerRowIsHeal[iRow];
        int iCopies = 0; bool bCanMerge = false; std::wstring wName = L"-";
        if (!bHeal)
        {
            auto pT = std::static_pointer_cast<Tower>(m_pTowerRowRefs[iRow].lock());
            if (pT)
            {
                const int wid = pT->GetWeaponId();
                const WeaponDef* pDef = WeaponDatabase::GetInst().Get(wid);
                wName = pDef ? TowerIntermissionUI_detail::ToW(pDef->strName) : L"None";
                auto vec = CollectAttackTowers(pT->GetTowerDefId());
                iCopies = static_cast<int>(vec.size());
                int iKeepLv = 1;
                for (auto& t : vec) if (t->GetLevel() > iKeepLv) iKeepLv = t->GetLevel();
                bCanMerge = iCopies >= 2 && iKeepLv < kMaxWeaponLevel;
            }
        }
        int iRefund = TowerIntermissionUI_detail::TowerBuyPrice(bHeal) / 2;
        if (iRefund < 1) iRefund = 1;

        // Panel layout under the tower row (mirrors OpenWeaponMenu).
        const Sizes S = ComputeSizes();
        const float Wpx = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float Hpx = static_cast<float>(Engine::Window::GetInst()->GetHeight());
        const float fRowH = S.fItemH;
        const float fW    = (std::max)(150.f, S.fW * 0.34f);
        const float fH    = fRowH * kMenuRows;
        const Rect& anchor = m_TowerRect[iRow];
        float px = anchor.x;
        float py = anchor.y + anchor.h + S.fGap;
        if (px + fW > Wpx) px = Wpx - fW;
        if (px < 0.f) px = 0.f;
        if (py + fH > Hpx) py = anchor.y - fH - S.fGap;   // flip above
        if (py < 0.f) py = 0.f;
        m_MenuPanelRect = { px, py, fW, fH };

        if (m_pMenuBg) { m_pMenuBg->SetRect(px, py, fW, fH); m_pMenuBg->Enable(); }
        for (int i = 0; i < kMenuRows; ++i)
        {
            const float ry = py + fRowH * i;
            if (m_pMenuButtons[i]) { m_pMenuButtons[i]->SetRect(px, ry, fW, fRowH); m_pMenuButtons[i]->Enable(); }
            if (m_pMenuTexts[i])   { m_pMenuTexts[i]->SetRect(px, ry, fW, fRowH);   m_pMenuTexts[i]->Enable(); }
        }
        // Rows: 0 Merge, 1 Weapon (cycle), 2 Sell.
        if (m_pMenuTexts[0])
        {
            m_pMenuTexts[0]->SetString(L"Merge  (x" + std::to_wstring(iCopies) + L")");
            m_pMenuTexts[0]->SetColor(bCanMerge ? 0x60FF60FFu : 0x808080FFu);
        }
        if (m_pMenuTexts[1])
        {
            m_pMenuTexts[1]->SetString(bHeal ? L"Weapon  (-)" : (L"Weapon: " + wName));
            m_pMenuTexts[1]->SetColor(bHeal ? 0x808080FFu : 0xFFFFFFFFu);
        }
        if (m_pMenuTexts[2])
        {
            m_pMenuTexts[2]->SetString(L"Sell  +" + std::to_wstring(iRefund) + L" G");
            m_pMenuTexts[2]->SetColor(0xFFD000FFu);
        }
    }

    void TowerIntermissionUI::OnTowerMenuMerge()
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        const int row = m_iMenuTowerRow;
        CloseWeaponMenu();
        if (row < 0 || row >= m_iTowerCount) return;
        if (m_bTowerRowIsHeal[row]) return;   // heal towers have no weapon level
        auto pClicked = std::static_pointer_cast<Tower>(m_pTowerRowRefs[row].lock());
        if (!pClicked) return;

        // Among the placed towers of the SAME TYPE (any weapon), keep the highest
        // level, consume the lowest, level the kept one up by one. Free — the cost
        // was the consumed tower (its owned slot is freed so it can be re-bought).
        // No-op if fewer than two or kept maxed.
        auto vec = CollectAttackTowers(pClicked->GetTowerDefId());
        if (vec.size() < 2) return;
        std::shared_ptr<Tower> pKeep = vec.front(), pErase;
        for (auto& t : vec) if (t->GetLevel() > pKeep->GetLevel()) pKeep = t;
        if (pKeep->GetLevel() >= kMaxWeaponLevel) return;   // maxed → don't waste a tower
        for (auto& t : vec)
            if (t != pKeep && (!pErase || t->GetLevel() < pErase->GetLevel())) pErase = t;
        if (!pErase) return;

        pErase->Despawn();
        TowerManager::GetInst().RemoveTower();   // free the owned slot (re-buyable)
        pKeep->SetLevel(pKeep->GetLevel() + 1);
        RebuildList();
    }

    void TowerIntermissionUI::OnTowerMenuCycle()
    {
        const int row = m_iMenuTowerRow;
        CloseWeaponMenu();
        if (row < 0) return;
        OnCycleTowerWeapon(row);   // menu closed + not armed → cycles the weapon
    }

    void TowerIntermissionUI::OnTowerMenuSell()
    {
        const int row = m_iMenuTowerRow;
        CloseWeaponMenu();
        if (row < 0) return;
        OnSellTower(row);
    }

    void TowerIntermissionUI::OnCycleTowerWeapon(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        if (iIndex < 0 || iIndex >= m_iTowerCount) return;
        if (m_bTowerRowIsHeal[iIndex])
        {
            // Heal towers have no weapon. Cancel a pending equip-arm so it
            // doesn't dangle, then ignore the click (R-click still sells).
            if (m_iEquipArmedWeaponId >= 0)
            {
                m_iEquipArmedWeaponId = -1;
                if (m_pDragGhost) m_pDragGhost->Disable();
            }
            return;
        }
        auto pTower = std::static_pointer_cast<Tower>(m_pTowerRowRefs[iIndex].lock());
        if (!pTower) return;

        // Armed by the weapon menu's Equip → assign that weapon, not a cycle.
        if (m_iEquipArmedWeaponId >= 0)
        {
            // Reject if another tower already holds it (one tower per weapon).
            if (!IsWeaponHeldByOtherTower(m_iEquipArmedWeaponId, pTower.get(), -1))
                pTower->SetWeaponId(m_iEquipArmedWeaponId);
            m_iEquipArmedWeaponId = -1;
            if (m_pDragGhost) m_pDragGhost->Disable();
            RebuildList();
            return;
        }

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Cycle this tower's weapon to the player's next owned weapon that no
        // OTHER tower already holds (each weapon sits on at most one tower).
        const std::vector<int> vecOwned = pPlayer->GetOwnedWeaponIds();
        const int sz = static_cast<int>(vecOwned.size());
        if (sz == 0) return;
        const int cur = pTower->GetWeaponId();
        int idx = -1;
        for (int i = 0; i < sz; ++i)
            if (vecOwned[i] == cur) { idx = i; break; }
        for (int step = 1; step <= sz; ++step)
        {
            const int cand = vecOwned[(idx + step) % sz];
            if (cand == cur) break;   // wrapped to current → nothing else free
            if (!IsWeaponHeldByOtherTower(cand, pTower.get(), -1))
            {
                pTower->SetWeaponId(cand);
                break;
            }
        }
        RebuildList();
    }

    void TowerIntermissionUI::OnSellTower(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        if (m_iEquipArmedWeaponId >= 0) return;   // an equip is armed → let the row equip, not sell
        if (iIndex < 0 || iIndex >= m_iTowerCount) return;
        auto pObj = m_pTowerRowRefs[iIndex].lock();
        if (!pObj) return;

        // Refund half the tower price (min 1), mirroring the weapon sell. Towers
        // are a flat price with no levels, so the refund is fixed. Both attack
        // and heal towers free their own owned-count slot (re-buyable).
        if (m_bTowerRowIsHeal[iIndex])
        {
            int iRefund = TowerIntermissionUI_detail::TowerBuyPrice(true) / 2;
            if (iRefund < 1) iRefund = 1;
            TowerManager::GetInst().RemoveHealTower();
            std::static_pointer_cast<HealTower>(pObj)->Despawn();
            Wallet::GetInst().Add(iRefund);
            RebuildList();
            return;
        }

        int iRefund = TowerIntermissionUI_detail::TowerBuyPrice(false) / 2;
        if (iRefund < 1) iRefund = 1;
        TowerManager::GetInst().RemoveTower();   // free the owned slot (re-buyable)
        std::static_pointer_cast<Tower>(pObj)->Despawn();   // leave the scene
        Wallet::GetInst().Add(iRefund);
        RebuildList();
    }

    void TowerIntermissionUI::OnCycleReserveWeapon(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        if (iIndex < 0 || iIndex >= m_iReserveCount) return;

        // Armed by the weapon menu's Equip → assign that weapon, not a cycle.
        if (m_iEquipArmedWeaponId >= 0)
        {
            // Reject if another tower already holds it (one tower per weapon).
            if (!IsWeaponHeldByOtherTower(m_iEquipArmedWeaponId, nullptr, iIndex))
                TowerManager::GetInst().SetReserveWeapon(iIndex, m_iEquipArmedWeaponId);
            m_iEquipArmedWeaponId = -1;
            if (m_pDragGhost) m_pDragGhost->Disable();
            RebuildList();
            return;
        }

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Cycle this unplaced tower's weapon to the player's next owned weapon
        // that no OTHER tower already holds (each weapon sits on one tower).
        const std::vector<int> vecOwned = pPlayer->GetOwnedWeaponIds();
        const int sz = static_cast<int>(vecOwned.size());
        if (sz == 0) return;
        // -1 = UNCONFIGURED (freshly bought, weaponless): the first click should
        // EQUIP a weapon (start scanning from index 0), not be treated as already
        // sitting on the default — otherwise a single-weapon player can never arm
        // the tower (and so can never place it).
        const int rawCur = TowerManager::GetInst().ReserveWeaponRaw(iIndex);
        int idx = -1;
        if (rawCur >= 0)
            for (int i = 0; i < sz; ++i)
                if (vecOwned[i] == rawCur) { idx = i; break; }
        for (int step = 1; step <= sz; ++step)
        {
            const int cand = vecOwned[(idx + step) % sz];
            if (rawCur >= 0 && cand == rawCur) break;   // wrapped to current → nothing else free
            if (!IsWeaponHeldByOtherTower(cand, nullptr, iIndex))
            {
                TowerManager::GetInst().SetReserveWeapon(iIndex, cand);
                break;
            }
        }
        RebuildList();
    }

    void TowerIntermissionUI::OnStart()
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        Hide();
        m_bShownLocal = false;
        if (m_fnStart) m_fnStart();
    }

    int TowerIntermissionUI::RerollCost() const
    {
        int iRound = m_fnRound ? m_fnRound() : 1;
        if (iRound < 1) iRound = 1;
        return kRerollBaseCost + (iRound - 1) * kRerollCostPerRound;
    }

    void TowerIntermissionUI::OnReroll()
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu

        // Nothing to do if every slot is pinned — don't charge for a no-op.
        int iRerollable = 0;
        for (int i = 0; i < kBuyRows; ++i)
            if (!m_bBuyLocked[i]) ++iRerollable;
        if (iRerollable == 0) return;

        if (!Wallet::GetInst().TrySpend(RerollCost())) return;   // can't afford
        RollCatalog();   // re-rolls all unpinned slots, keeps pinned ones
        RebuildList();
    }

    void TowerIntermissionUI::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        const bool bInter =
            GameStateManager::GetInst().GetState() == GameState::Intermission;

        if (bInter && !m_bShownLocal)
        {
            RollCatalog();
            RebuildList();
            Show();
            m_bShownLocal = true;
        }
        else if (!bInter && m_bShownLocal)
        {
            Hide();
            m_bShownLocal = false;
        }

        if (m_bShownLocal)
        {
            HandleDrag();
            HandleWeaponMenu();
            if (m_iDragWeaponId < 0) HandleTooltip();   // no tooltip mid-drag
        }
    }

    void TowerIntermissionUI::HandleDrag()
    {
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        auto* pInput = Engine::CInput::GetInst();
        using MOUSE = Engine::CInput::MOUSE_TYPE;
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());

        // --- Begin: press over a filled equipped / inventory icon picks it up.
        if (m_iDragWeaponId < 0)
        {
            // Don't START a drag while the menu's click-to-arm equip flow or any
            // menu owns the cursor (an in-progress drag still resolves below).
            if (m_iEquipArmedWeaponId >= 0) return;
            if (m_iMenuWeaponId >= 0 || m_iMenuTowerRow >= 0) return;
            if (!pInput->IsMouseButtonDown(MOUSE::LEFT)) return;
            for (int i = 0; i < kOwnedRows; ++i)
                if (m_iOwnedIds[i] >= 0 && InRect(mx, my, m_OwnedRect[i]))
                { m_iDragWeaponId = m_iOwnedIds[i]; m_eDragSrc = DragSrc::Equipped; break; }
            if (m_iDragWeaponId < 0)
                for (int i = 0; i < kInvRows; ++i)
                    if (m_iInvIds[i] >= 0 && InRect(mx, my, m_InvRect[i]))
                    { m_iDragWeaponId = m_iInvIds[i]; m_eDragSrc = DragSrc::Inventory; break; }
            if (m_iDragWeaponId >= 0 && m_pDragGhost)
            {
                const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iDragWeaponId);
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x808080;
                m_pDragGhost->SetTexture(TowerIntermissionUI_detail::EnsureSolidTexture(
                    "tower_shop_w_" + std::to_string(m_iDragWeaponId), uColor));
            }
            return;
        }

        // --- Dragging: the ghost trails the cursor.
        const float fGhost = m_OwnedRect[0].h;
        if (m_pDragGhost)
        {
            m_pDragGhost->SetRect(mx - fGhost * 0.5f, my - fGhost * 0.5f, fGhost, fGhost);
            m_pDragGhost->Enable();
        }
        if (!pInput->IsMouseButtonUp(MOUSE::LEFT)) return;   // still held

        // --- Release: resolve the drop target, then clear the drag.
        const int     iDragId = m_iDragWeaponId;
        const DragSrc eSrc    = m_eDragSrc;
        m_iDragWeaponId = -1; m_eDragSrc = DragSrc::None;
        if (m_pDragGhost) m_pDragGhost->Disable();

        // 1) Onto a placed tower row → equip it there (reuse the arm path's
        //    one-tower-per-weapon check + assignment).
        for (int i = 0; i < m_iTowerCount; ++i)
            if (InRect(mx, my, m_TowerRect[i]))
            {
                if (!m_bTowerRowIsHeal[i]) { m_iEquipArmedWeaponId = iDragId; OnCycleTowerWeapon(i); }
                return;
            }
        // 2) Onto an unplaced (reserve) tower slot → equip it there.
        for (int i = 0; i < m_iReserveCount; ++i)
            if (InRect(mx, my, m_ReserveRect[i]))
            { m_iEquipArmedWeaponId = iDragId; OnCycleReserveWeapon(i); return; }
        // 3) Onto the equipped strip → equip (only meaningful from inventory).
        const int iEquipCap = Player::GetMaxEquipSlots();
        for (int i = 0; i < kOwnedRows && i < iEquipCap; ++i)
            if (InRect(mx, my, m_OwnedRect[i]))
            {
                if (eSrc == DragSrc::Inventory) { pPlayer->EquipWeapon(iDragId); RebuildList(); }
                return;
            }
        // 4) Onto the inventory strip → unequip (only meaningful from equipped).
        for (int i = 0; i < kInvRows; ++i)
            if (InRect(mx, my, m_InvRect[i]))
            {
                if (eSrc == DragSrc::Equipped) { pPlayer->UnequipWeapon(iDragId); RebuildList(); }
                return;
            }
        // Dropped elsewhere → no change.
    }

    void TowerIntermissionUI::HandleWeaponMenu()
    {
        auto* pInput = Engine::CInput::GetInst();
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());
        using MOUSE = Engine::CInput::MOUSE_TYPE;

        // Equip-arm: a ghost follows the cursor; the actual assignment happens
        // in OnCycleTowerWeapon / OnCycleReserveWeapon (their button OnClick
        // fires during the input dispatch above). A left-click that didn't land
        // on a tower row / reserve slot cancels the arm.
        if (m_iEquipArmedWeaponId >= 0)
        {
            const float fGhost = m_OwnedRect[0].h;   // square, row height
            if (m_pDragGhost)
                m_pDragGhost->SetRect(mx - fGhost * 0.5f, my - fGhost * 0.5f, fGhost, fGhost);

            // The click that armed this (on the Equip button) lands the same
            // frame — don't treat it as an off-target cancel.
            if (m_bEquipArmedThisFrame)
            {
                m_bEquipArmedThisFrame = false;
                return;
            }

            if (pInput->IsMouseButtonDown(MOUSE::LEFT))
            {
                bool bOnTarget = false;
                for (int i = 0; i < m_iTowerCount; ++i)
                    if (InRect(mx, my, m_TowerRect[i])) { bOnTarget = true; break; }
                if (!bOnTarget)
                    for (int i = 0; i < m_iReserveCount; ++i)
                        if (InRect(mx, my, m_ReserveRect[i])) { bOnTarget = true; break; }
                // On-target clicks are consumed by the row buttons' OnClick;
                // anything else cancels the arm.
                if (!bOnTarget)
                {
                    m_iEquipArmedWeaponId = -1;
                    if (m_pDragGhost) m_pDragGhost->Disable();
                }
            }
            return;   // armed: no menu open at the same time
        }

        // Menu dismissal: a left-click outside the menu panel (and not on an
        // owned icon, which would just reopen it) closes the menu. Menu-button
        // clicks already closed it via their handlers during input dispatch, so
        // by here m_iMenuWeaponId is -1 for those.
        if ((m_iMenuWeaponId >= 0 || m_iMenuTowerRow >= 0) && pInput->IsMouseButtonDown(MOUSE::LEFT))
        {
            bool bInside = InRect(mx, my, m_MenuPanelRect);
            // A click on the anchor row (owned icon for the weapon menu, tower
            // row for the tower menu) just reopens it — don't treat as dismiss.
            if (!bInside && m_iMenuWeaponId >= 0)
                for (int i = 0; i < kOwnedRows; ++i)
                    if (m_iOwnedIds[i] >= 0 && InRect(mx, my, m_OwnedRect[i])) { bInside = true; break; }
            if (!bInside && m_iMenuTowerRow >= 0)
                for (int i = 0; i < m_iTowerCount; ++i)
                    if (InRect(mx, my, m_TowerRect[i])) { bInside = true; break; }
            if (!bInside) CloseWeaponMenu();
        }
    }

    void TowerIntermissionUI::HandleTooltip()
    {
        using namespace TowerIntermissionUI_detail;

        auto hideTip = [&]()
        {
            if (m_pTooltipBg)   m_pTooltipBg->Disable();
            if (m_pTooltipText) m_pTooltipText->Disable();
        };
        // While the action menu is open or a weapon is armed for equip, the
        // cursor is busy — suppress the hover tooltip.
        if (m_iMenuWeaponId >= 0 || m_iMenuTowerRow >= 0 || m_iEquipArmedWeaponId >= 0) { hideTip(); return; }

        auto* pInput = Engine::CInput::GetInst();
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());
        auto pPlayer = m_pTarget.lock();

        // What's under the cursor? Buy rows first (weapon OR tower kind), then
        // the owned-weapon strip. Weapon stats show at the player's owned level
        // (so a levelled weapon reads its real numbers); fall back to level 1.
        // Towers aren't WeaponDefs, so they get a fixed tower description.
        auto weaponTip = [&](int id) -> std::wstring
        {
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            if (!pDef) return L"";
            int iLevel = pPlayer ? pPlayer->GetOwnedWeaponLevel(id) : 0;
            if (iLevel < 1) iLevel = 1;
            return BuildWeaponTooltip(*pDef, iLevel);
        };

        std::wstring wInfo;
        for (int i = 0; i < kBuyRows; ++i)
        {
            if (!m_bBuyUsed[i] || !InRect(mx, my, m_BuyRect[i])) continue;
            if (m_eBuyKind[i] == BuyKind::Weapon)
                wInfo = weaponTip(m_iBuyIds[i]);
            else if (m_eBuyKind[i] == BuyKind::HealTower)
                wInfo = BuildTowerTooltip(true);
            else
                wInfo = BuildAttackTowerTooltip(m_iBuyIds[i]);
            break;
        }
        if (wInfo.empty())
            for (int i = 0; i < kOwnedRows; ++i)
            {
                if (m_iOwnedIds[i] < 0 || !InRect(mx, my, m_OwnedRect[i])) continue;
                wInfo = weaponTip(m_iOwnedIds[i]);
                break;
            }
        // Placed-tower loadout rows: show the tower's own stats. Heal rows reuse
        // the heal description; attack rows show the attack-tower stats plus the
        // specific weapon this tower is firing.
        if (wInfo.empty())
            for (int i = 0; i < m_iTowerCount; ++i)
            {
                if (!InRect(mx, my, m_TowerRect[i])) continue;
                if (m_bTowerRowIsHeal[i])
                {
                    wInfo = BuildTowerTooltip(true);
                }
                else
                {
                    wInfo = BuildTowerTooltip(false);
                    if (auto pT = std::static_pointer_cast<Tower>(m_pTowerRowRefs[i].lock()))
                    {
                        const WeaponDef* pW = WeaponDatabase::GetInst().Get(pT->GetWeaponId());
                        wInfo += L"\nWeapon: " + (pW ? ToW(pW->strName) : std::wstring(L"None"));
                    }
                }
                break;
            }
        // Unplaced-tower (reserve) icons: each shows the weapon that tower will
        // fire — hover shows that weapon's stats, resolving the -1 "inherit the
        // current default" the same way the icon colour does.
        if (wInfo.empty())
            for (int i = 0; i < m_iReserveCount; ++i)
            {
                if (!InRect(mx, my, m_ReserveRect[i])) continue;
                int wid = TowerManager::GetInst().ReserveWeaponRaw(i);
                if (wid < 0) wid = TowerManager::GetInst().CurrentWeaponId();
                wInfo = weaponTip(wid);
                break;
            }

        if (wInfo.empty()) { hideTip(); return; }

        // Size the panel to the line count; place it next to the cursor and clamp
        // to the window so it never spills off-screen.
        const Sizes S = ComputeSizes();
        const float Wpx = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float Hpx = static_cast<float>(Engine::Window::GetInst()->GetHeight());
        int iLines = 1;
        for (wchar_t c : wInfo) if (c == L'\n') ++iLines;
        const float fPad   = (std::max)(4.f, S.fGap * 1.5f);
        const float fLineH = (std::max)(14.f, S.fItemH * 0.62f);
        const float fTipW  = S.fW * 0.9f;
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

    std::shared_ptr<Engine::Component> TowerIntermissionUI::Clone()
    {
        return std::make_shared<TowerIntermissionUI>(*this);
    }
}

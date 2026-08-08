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
        if (m_pOwnedHeader) m_pOwnedHeader->SetString(L"Equipped (fires) - double-click: unequip / R-click: menu");
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
                    // Double-click unequips (→ inventory); R-click pops the menu.
                    m_pOwnedIcons[i]->SetOnClick([this, idx]() { OnWeaponIconClick(true, idx); });
                    m_pOwnedIcons[i]->SetOnRightClick([this, idx]() { OpenWeaponMenu(m_iOwnedIds[idx], m_OwnedRect[idx]); });
                }
                // Per-copy level badge, bottom-right of the icon.
                m_pOwnedLvlTexts[i] = makeText("tower_shop_owned_lv" + std::to_string(i), y, fIconH, m_pItemFont, Engine::Text::HAlign::Right);
                if (m_pOwnedLvlTexts[i]) m_pOwnedLvlTexts[i]->SetRect(fX, y + fIconH * 0.55f, fIconW - fIconH * 0.08f, fIconH * 0.4f);
            }
            y += fIconH + S.fGap;
        }

        // --- Weapon inventory (idle, unassigned) — horizontal icon strip ---
        m_pInvHeader = makeText("tower_shop_inv_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pInvHeader) m_pInvHeader->SetString(L"Inventory (idle) - double-click: equip / R-click: menu");
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
                    // Double-click equips (→ a free firing slot); R-click = menu.
                    m_pInvIcons[i]->SetOnClick([this, idx]() { OnWeaponIconClick(false, idx); });
                    m_pInvIcons[i]->SetOnRightClick([this, idx]() { OpenWeaponMenu(m_iInvIds[idx], m_InvRect[idx]); });
                }
                // Per-copy level badge, bottom-right of the icon.
                m_pInvLvlTexts[i] = makeText("tower_shop_inv_lv" + std::to_string(i), y, fIconH, m_pItemFont, Engine::Text::HAlign::Right);
                if (m_pInvLvlTexts[i]) m_pInvLvlTexts[i]->SetRect(fX, y + fIconH * 0.55f, fIconW - fIconH * 0.08f, fIconH * 0.4f);
            }
            y += fIconH + S.fGap;
        }

        // --- Tower Loadout section (drop targets) ---
        m_pTowerHeader = makeText("tower_shop_tower_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pTowerHeader) m_pTowerHeader->SetString(L"Towers - click for menu (merge / weapon / sell)");
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
                // One unified list of placed + unplaced towers. Equip armed (weapon
                // menu) → assign that weapon here; otherwise pop the action menu.
                m_pTowerButtons[i]->SetOnClick([this, idx]() { OnTowerRowClick(idx); });
                // Right-click pops the action menu too (Merge / Weapon / Sell) —
                // it no longer sells directly, so an accidental R-click can't
                // dump a tower; selling is the menu's Sell row.
                m_pTowerButtons[i]->SetOnRightClick([this, idx]() { OpenTowerMenu(idx); });
            }
            m_pTowerTexts[i] = makeText("tower_shop_tower_t" + std::to_string(i), y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);
            y += S.fItemH + S.fGap;
        }

        // (Unplaced towers used to live in a separate icon strip here; they now
        // share the single tower list above — placed and unplaced are no longer
        // distinguished — so that section is gone and Start moves up.)

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
        {
            if (m_pOwnedIcons[i])    m_pOwnedIcons[i]->Disable();
            if (m_pOwnedLvlTexts[i]) m_pOwnedLvlTexts[i]->Disable();
        }
        for (int i = 0; i < kInvRows; ++i)
        {
            if (m_pInvIcons[i])    m_pInvIcons[i]->Disable();
            if (m_pInvLvlTexts[i]) m_pInvLvlTexts[i]->Disable();
        }
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
        // (Tower-held weapons are no longer tracked by id: a weapon assigned to a
        // tower is MOVED out of the player's lists entirely, so the equipped /
        // inventory lists below are already exact — no sync needed.)
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
        const std::vector<int> vecEquippedLv =
            pPlayer ? pPlayer->GetEquippedWeaponLevels() : std::vector<int>{};
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
            // Per-copy level badge: this icon's OWN level (copies of the same
            // weapon can differ), shown only on a filled firing slot.
            if (m_pOwnedLvlTexts[i])
            {
                if (bHas && i < static_cast<int>(vecEquippedLv.size()))
                {
                    m_pOwnedLvlTexts[i]->SetString(L"Lv." + std::to_wstring(vecEquippedLv[i]));
                    m_pOwnedLvlTexts[i]->Enable();
                }
                else m_pOwnedLvlTexts[i]->Disable();
            }
        }

        // --- Weapon inventory (idle) ---
        const std::vector<int> vecInv =
            pPlayer ? pPlayer->GetInventoryWeaponIds() : std::vector<int>{};
        const std::vector<int> vecInvLv =
            pPlayer ? pPlayer->GetInventoryWeaponLevels() : std::vector<int>{};
        for (int i = 0; i < kInvRows; ++i)
        {
            if (!m_pInvIcons[i]) continue;
            const bool bHas = i < static_cast<int>(vecInv.size());
            const int  id   = bHas ? vecInv[i] : -1;
            m_iInvIds[i] = id;
            if (!bHas)
            {
                m_pInvIcons[i]->Disable();
                if (m_pInvLvlTexts[i]) m_pInvLvlTexts[i]->Disable();
                continue;
            }
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
            m_pInvIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
            m_pInvIcons[i]->Enable();
            if (m_pInvLvlTexts[i])
            {
                m_pInvLvlTexts[i]->SetString(
                    L"Lv." + std::to_wstring(i < static_cast<int>(vecInvLv.size()) ? vecInvLv[i] : 1));
                m_pInvLvlTexts[i]->Enable();
            }
        }

        // --- Towers (placed + unplaced in ONE list, acquisition order) --------
        // Placed and unplaced towers are no longer distinguished: every owned
        // tower (attack or heal) gets one row, ordered by acquisition seq (same
        // order as the in-game tower HUD). Each row records its source (a placed
        // scene object or a reserve index) so the action menu / drag can act on
        // it. Clicking a row opens the Merge / Weapon / Sell menu.
        struct Row {
            int seq; bool heal; int defId; int weapon; int level;
            TowerRowSrc src; std::shared_ptr<Engine::GameObject> obj; int reserveIdx;
        };
        std::vector<Row> vecRows;
        {
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pLayer)
                for (const auto& p : pLayer->GetGameObjectList())
                {
                    if (!p || !p->IsActive()) continue;
                    if (p->GetTag() == "Tower")
                    {
                        auto pT = std::static_pointer_cast<Tower>(p);
                        vecRows.push_back({ pT->GetSlotSeq(), false, pT->GetTowerDefId(),
                            pT->GetWeaponId(), pT->GetLevel(), TowerRowSrc::Placed, p, -1 });
                    }
                    else if (p->GetTag() == "HealTower")
                    {
                        auto pH = std::static_pointer_cast<HealTower>(p);
                        vecRows.push_back({ pH->GetSlotSeq(), true, -1, -1, pH->GetLevel(),
                            TowerRowSrc::Placed, p, -1 });
                    }
                }
        }
        {
            auto& tmgr = TowerManager::GetInst();
            for (int i = 0; i < tmgr.ReserveCount(); ++i)
                vecRows.push_back({ tmgr.ReserveSeq(i), false, tmgr.ReserveTowerId(i),
                    tmgr.ReserveWeaponRaw(i), tmgr.ReserveLevel(i), TowerRowSrc::AtkReserve, nullptr, i });
            for (int i = 0; i < tmgr.HealReserveCount(); ++i)
                vecRows.push_back({ tmgr.HealReserveSeq(i), true, -1, -1, tmgr.HealReserveLevel(i),
                    TowerRowSrc::HealReserve, nullptr, i });
        }
        std::sort(vecRows.begin(), vecRows.end(),
            [](const Row& a, const Row& b) { return a.seq < b.seq; });

        m_iTowerCount = (std::min)(kTowerRows, static_cast<int>(vecRows.size()));
        for (int i = 0; i < kTowerRows; ++i)
        {
            const bool bHas = i < m_iTowerCount;
            if (!bHas)
            {
                m_pTowerRowRefs[i].reset();
                m_bTowerRowIsHeal[i]  = false;
                m_eTowerRowSrc[i]     = TowerRowSrc::Placed;
                m_iTowerRowReserve[i] = -1;
                m_iTowerRowDefId[i]   = -1;
                m_iTowerRowLevel[i]   = 1;
                if (m_pTowerButtons[i]) m_pTowerButtons[i]->Disable();
                if (m_pTowerTexts[i])   m_pTowerTexts[i]->Disable();
                continue;
            }
            const Row& r = vecRows[i];
            m_eTowerRowSrc[i]     = r.src;
            m_bTowerRowIsHeal[i]  = r.heal;
            m_iTowerRowReserve[i] = r.reserveIdx;
            m_iTowerRowDefId[i]   = r.defId;
            m_iTowerRowLevel[i]   = r.level;
            if (r.src == TowerRowSrc::Placed) m_pTowerRowRefs[i] = r.obj;
            else                              m_pTowerRowRefs[i].reset();

            if (r.heal)
            {
                if (m_pTowerButtons[i])
                {
                    m_pTowerButtons[i]->SetTexture(EnsureSolidTexture("tower_shop_healrow", 0x1E7A3C));
                    m_pTowerButtons[i]->Enable();
                }
                if (m_pTowerTexts[i])
                {
                    m_pTowerTexts[i]->SetColor(0x80FF80FFu);
                    m_pTowerTexts[i]->SetString(L"Heal Tower Lv." + std::to_wstring(r.level));
                    m_pTowerTexts[i]->Enable();
                }
                continue;
            }

            // Attack tower: type name + level + equipped weapon (or "(no weapon)").
            // Box tinted by the equipped weapon's colour (grey when unequipped).
            const TowerDef* pTD = (r.defId >= 0)
                ? TowerDatabase::GetInst().Get(r.defId)
                : TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
            const std::wstring wType = pTD ? ToW(pTD->strName) : std::wstring(L"Tower");
            const WeaponDef* pWDef = (r.weapon >= 0) ? WeaponDatabase::GetInst().Get(r.weapon) : nullptr;
            const std::wstring wName = pWDef ? ToW(pWDef->strName) : std::wstring(L"(no weapon)");
            if (m_pTowerButtons[i])
            {
                const unsigned int uColor = pWDef ? pWDef->uColorRGB : 0x484848u;
                const std::string tag = pWDef
                    ? ("tower_shop_w_" + std::to_string(r.weapon))
                    : std::string("tower_shop_tower_nowpn");
                m_pTowerButtons[i]->SetTexture(EnsureSolidTexture(tag, uColor));
                m_pTowerButtons[i]->Enable();
            }
            if (m_pTowerTexts[i])
            {
                m_pTowerTexts[i]->SetColor(0xFFFFFFFFu);
                m_pTowerTexts[i]->SetString(
                    wType + L" Lv." + std::to_wstring(r.level) + L"  [" + wName + L"]");
                m_pTowerTexts[i]->Enable();
            }
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

    void TowerIntermissionUI::OnWeaponIconClick(bool bEquipped, int iIndex)
    {
        // Only a DOUBLE-click (a quick second click on the same icon) toggles
        // equip/unequip; a single click is ignored so it can't fire by accident.
        // The gap is measured in frames (intermission dt ~ 0; see m_iClickFrame).
        const int iKey = (bEquipped ? 1 : 2) * 1000 + iIndex;
        const bool bDouble = (iKey == m_iLastClickKey) &&
                             (m_iClickFrame - m_iLastClickFrame) <= kDoubleClickFrames;
        if (!bDouble)
        {
            // First click: just remember it (and when) so the next one can pair.
            m_iLastClickKey   = iKey;
            m_iLastClickFrame = m_iClickFrame;
            return;
        }
        // Second quick click → act, then reset so a third click starts fresh.
        m_iLastClickKey = -1;
        if (bEquipped) OnEquipSlotClick(iIndex);
        else           OnInventoryClick(iIndex);
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
    int TowerIntermissionUI::ResolveTowerType(int iTowerDefId) const
    {
        if (iTowerDefId >= 0) return iTowerDefId;
        const TowerDef* pDef = TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
        return pDef ? pDef->iId : -1;
    }

    void TowerIntermissionUI::CountTowersOfType(int iType, int& outCount, int& outMaxLevel) const
    {
        outCount = 0; outMaxLevel = 1;
        // Placed attack towers of this resolved type.
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (pLayer)
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
                auto pT = std::static_pointer_cast<Tower>(p);
                if (ResolveTowerType(pT->GetTowerDefId()) != iType) continue;
                ++outCount;
                if (pT->GetLevel() > outMaxLevel) outMaxLevel = pT->GetLevel();
            }
        // Unplaced (reserve) attack towers of this resolved type.
        auto& tm = TowerManager::GetInst();
        for (int i = 0; i < tm.ReserveCount(); ++i)
        {
            if (ResolveTowerType(tm.ReserveTowerId(i)) != iType) continue;
            ++outCount;
            if (tm.ReserveLevel(i) > outMaxLevel) outMaxLevel = tm.ReserveLevel(i);
        }
    }

    void TowerIntermissionUI::CountHealTowers(int& outCount, int& outMaxLevel) const
    {
        outCount = 0; outMaxLevel = 1;
        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (pLayer)
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "HealTower") continue;
                auto pH = std::static_pointer_cast<HealTower>(p);
                ++outCount;
                if (pH->GetLevel() > outMaxLevel) outMaxLevel = pH->GetLevel();
            }
        auto& tm = TowerManager::GetInst();
        for (int i = 0; i < tm.HealReserveCount(); ++i)
        {
            ++outCount;
            if (tm.HealReserveLevel(i) > outMaxLevel) outMaxLevel = tm.HealReserveLevel(i);
        }
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

        // Merge needs >= 2 towers of the same TYPE (placed OR unplaced, any
        // weapon) and a kept copy below the cap; heal towers never merge.
        const bool bHeal = m_bTowerRowIsHeal[iRow];
        int iCopies = 0; bool bCanMerge = false; std::wstring wName = L"-";
        if (!bHeal)
        {
            // Equipped weapon name — from the placed object or the reserve entry.
            int wid = -1;
            if (m_eTowerRowSrc[iRow] == TowerRowSrc::Placed)
            {
                if (auto pT = std::static_pointer_cast<Tower>(m_pTowerRowRefs[iRow].lock()))
                    wid = pT->GetWeaponId();
            }
            else
            {
                wid = TowerManager::GetInst().ReserveWeaponRaw(m_iTowerRowReserve[iRow]);
            }
            const WeaponDef* pDef = (wid >= 0) ? WeaponDatabase::GetInst().Get(wid) : nullptr;
            wName = pDef ? TowerIntermissionUI_detail::ToW(pDef->strName) : std::wstring(L"None");

            int iMaxLv = 1;
            CountTowersOfType(ResolveTowerType(m_iTowerRowDefId[iRow]), iCopies, iMaxLv);
            bCanMerge = iCopies >= 2 && iMaxLv < kMaxWeaponLevel;
        }
        else
        {
            // Heal towers are all one type — any two merge (keep level +1).
            int iMaxLv = 1;
            CountHealTowers(iCopies, iMaxLv);
            bCanMerge = iCopies >= 2 && iMaxLv < kMaxWeaponLevel;
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

        // --- Heal towers: all one type, no weapon. Merge any two (placed or
        // unplaced) — keep the highest level +1, consume the other. ----------
        if (m_bTowerRowIsHeal[row])
        {
            struct HH { bool reserve; std::shared_ptr<HealTower> placed; int rIdx; int level; };
            std::vector<HH> hs;
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pLayer)
                for (const auto& p : pLayer->GetGameObjectList())
                {
                    if (!p || !p->IsActive() || p->GetTag() != "HealTower") continue;
                    auto pH = std::static_pointer_cast<HealTower>(p);
                    hs.push_back({ false, pH, -1, pH->GetLevel() });
                }
            auto& tmH = TowerManager::GetInst();
            for (int i = 0; i < tmH.HealReserveCount(); ++i)
                hs.push_back({ true, nullptr, i, tmH.HealReserveLevel(i) });

            if (hs.size() < 2) return;
            int iKeepH = 0;
            for (int k = 1; k < static_cast<int>(hs.size()); ++k)
                if (hs[k].level > hs[iKeepH].level) iKeepH = k;
            if (hs[iKeepH].level >= kMaxWeaponLevel) return;
            int iConsH = -1;
            for (int k = 0; k < static_cast<int>(hs.size()); ++k)
                if (k != iKeepH && (iConsH < 0 || hs[k].level < hs[iConsH].level)) iConsH = k;
            if (iConsH < 0) return;

            // Bump kept first (reserve index still valid), then remove consumed.
            if (hs[iKeepH].reserve) tmH.AddHealReserveLevel(hs[iKeepH].rIdx, 1);
            else                    hs[iKeepH].placed->SetLevel(hs[iKeepH].placed->GetLevel() + 1);
            if (hs[iConsH].reserve) tmH.RemoveHealReserveAt(hs[iConsH].rIdx);
            else { hs[iConsH].placed->Despawn(); tmH.RemoveHealTower(); }
            RebuildList();
            return;
        }

        const int iType = ResolveTowerType(m_iTowerRowDefId[row]);

        // Gather ALL owned towers of this TYPE — placed AND unplaced, regardless
        // of equipped weapon. Keep the highest level, consume the lowest, level
        // the kept one up by one. Free (the cost was the consumed tower).
        struct H { bool reserve; std::shared_ptr<Tower> placed; int rIdx; int level; };
        std::vector<H> hs;
        {
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pLayer)
                for (const auto& p : pLayer->GetGameObjectList())
                {
                    if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
                    auto pT = std::static_pointer_cast<Tower>(p);
                    if (ResolveTowerType(pT->GetTowerDefId()) == iType)
                        hs.push_back({ false, pT, -1, pT->GetLevel() });
                }
        }
        auto& tm = TowerManager::GetInst();
        for (int i = 0; i < tm.ReserveCount(); ++i)
            if (ResolveTowerType(tm.ReserveTowerId(i)) == iType)
                hs.push_back({ true, nullptr, i, tm.ReserveLevel(i) });

        if (hs.size() < 2) return;
        int iKeep = 0;
        for (int k = 1; k < static_cast<int>(hs.size()); ++k)
            if (hs[k].level > hs[iKeep].level) iKeep = k;
        if (hs[iKeep].level >= kMaxWeaponLevel) return;   // maxed → don't waste a tower
        int iCons = -1;
        for (int k = 0; k < static_cast<int>(hs.size()); ++k)
            if (k != iKeep && (iCons < 0 || hs[k].level < hs[iCons].level)) iCons = k;
        if (iCons < 0) return;

        // Bump the kept copy FIRST (its reserve index is still valid), then remove
        // the consumed one (erasing a reserve entry can shift later indices). The
        // consumed tower's weapon goes back to the player (never destroyed).
        auto pPlayer = m_pTarget.lock();
        if (hs[iKeep].reserve) tm.AddReserveLevel(hs[iKeep].rIdx, 1);
        else                   hs[iKeep].placed->SetLevel(hs[iKeep].placed->GetLevel() + 1);
        if (hs[iCons].reserve)
        {
            if (pPlayer) if (WeaponPtr w = tm.ReserveWeapon(hs[iCons].rIdx)) pPlayer->AttachWeapon(w);
            tm.RemoveReserveTower(hs[iCons].rIdx);
        }
        else
        {
            if (pPlayer) if (WeaponPtr w = hs[iCons].placed->ReleaseWeapon()) pPlayer->AttachWeapon(w);
            hs[iCons].placed->Despawn();
            tm.RemoveTower();
        }
        RebuildList();
    }

    void TowerIntermissionUI::OnTowerMenuCycle()
    {
        const int row = m_iMenuTowerRow;
        const TowerRowSrc src = (row >= 0 && row < kTowerRows) ? m_eTowerRowSrc[row] : TowerRowSrc::Placed;
        const int rIdx = (row >= 0 && row < kTowerRows) ? m_iTowerRowReserve[row] : -1;
        CloseWeaponMenu();
        if (row < 0) return;
        // Menu closed + not armed → cycles the weapon (placed vs reserve path).
        if (src == TowerRowSrc::Placed)         OnCycleTowerWeapon(row);
        else if (src == TowerRowSrc::AtkReserve) OnCycleReserveWeapon(rIdx);
        // Heal reserve: no weapon to cycle.
    }

    void TowerIntermissionUI::OnTowerMenuSell()
    {
        const int row = m_iMenuTowerRow;
        CloseWeaponMenu();
        if (row < 0) return;
        OnTowerRowSell(row);
    }

    void TowerIntermissionUI::OnTowerRowClick(int iRow)
    {
        if (iRow < 0 || iRow >= m_iTowerCount) return;
        // An armed weapon (weapon menu's Equip / a drag) assigns to this tower.
        if (m_iEquipArmedWeaponId >= 0)
        {
            const TowerRowSrc src = m_eTowerRowSrc[iRow];
            if (src == TowerRowSrc::Placed && !m_bTowerRowIsHeal[iRow])
                OnCycleTowerWeapon(iRow);
            else if (src == TowerRowSrc::AtkReserve)
                OnCycleReserveWeapon(m_iTowerRowReserve[iRow]);
            else
            {
                // Heal tower: nothing to equip — cancel the arm so it doesn't dangle.
                m_iEquipArmedWeaponId = -1;
                if (m_pDragGhost) m_pDragGhost->Disable();
            }
            return;
        }
        OpenTowerMenu(iRow);
    }

    void TowerIntermissionUI::OnTowerRowSell(int iRow)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;
        if (m_iEquipArmedWeaponId >= 0) return;   // armed → row equips, not sells
        if (iRow < 0 || iRow >= m_iTowerCount) return;

        const bool bHeal = m_bTowerRowIsHeal[iRow];
        int iRefund = TowerIntermissionUI_detail::TowerBuyPrice(bHeal) / 2;
        if (iRefund < 1) iRefund = 1;

        switch (m_eTowerRowSrc[iRow])
        {
        case TowerRowSrc::Placed:
            OnSellTower(iRow);   // existing placed-tower sell (Despawn + refund)
            return;
        case TowerRowSrc::AtkReserve:
        {
            // Hand the reserve tower's weapon back to the player before removing it.
            const int rIdx = m_iTowerRowReserve[iRow];
            if (auto pPlayer = m_pTarget.lock())
                if (WeaponPtr w = TowerManager::GetInst().ReserveWeapon(rIdx))
                    pPlayer->AttachWeapon(w);
            TowerManager::GetInst().RemoveReserveTower(rIdx);
            Wallet::GetInst().Add(iRefund);
            RebuildList();
            return;
        }
        case TowerRowSrc::HealReserve:
            TowerManager::GetInst().RemoveHealReserveAt(m_iTowerRowReserve[iRow]);
            Wallet::GetInst().Add(iRefund);
            RebuildList();
            return;
        }
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
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Armed by the weapon menu's Equip / a drag → MOVE a player copy of that
        // weapon onto this tower; the tower's previous weapon (if any) returns to
        // the player inventory (a weapon object lives in exactly one place).
        if (m_iEquipArmedWeaponId >= 0)
        {
            if (WeaponPtr w = pPlayer->DetachWeapon(m_iEquipArmedWeaponId))
            {
                WeaponPtr old = pTower->ReleaseWeapon();
                pTower->SetWeapon(w);
                if (old) pPlayer->AttachWeapon(old);
            }
            m_iEquipArmedWeaponId = -1;
            if (m_pDragGhost) m_pDragGhost->Disable();
            RebuildList();
            return;
        }

        // Plain click (not armed) → UNASSIGN: hand the tower's weapon back to the
        // player inventory. Re-assign a weapon via its Equip action / drag.
        if (WeaponPtr old = pTower->ReleaseWeapon())
            pPlayer->AttachWeapon(old);
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
        auto pT = std::static_pointer_cast<Tower>(pObj);
        // Hand the tower's weapon back to the player before it leaves.
        if (auto pPlayer = m_pTarget.lock())
            if (WeaponPtr w = pT->ReleaseWeapon()) pPlayer->AttachWeapon(w);
        TowerManager::GetInst().RemoveTower();   // free the owned slot (re-buyable)
        pT->Despawn();                           // leave the scene
        Wallet::GetInst().Add(iRefund);
        RebuildList();
    }

    void TowerIntermissionUI::OnCycleReserveWeapon(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (PointerInOpenMenu()) return;   // click belongs to the open weapon menu
        // Indexed against the live reserve (the unplaced-tower icon strip that
        // used to bound this was removed — towers share one list now).
        if (iIndex < 0 || iIndex >= TowerManager::GetInst().ReserveCount()) return;

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Armed → MOVE a player copy onto this unplaced tower; its previous
        // weapon (if any) returns to the player inventory.
        if (m_iEquipArmedWeaponId >= 0)
        {
            if (WeaponPtr w = pPlayer->DetachWeapon(m_iEquipArmedWeaponId))
            {
                WeaponPtr old = TowerManager::GetInst().SetReserveWeapon(iIndex, w);
                if (old) pPlayer->AttachWeapon(old);
            }
            m_iEquipArmedWeaponId = -1;
            if (m_pDragGhost) m_pDragGhost->Disable();
            RebuildList();
            return;
        }

        // Plain click → UNASSIGN: hand the reserve tower's weapon back to inventory.
        if (WeaponPtr old = TowerManager::GetInst().SetReserveWeapon(iIndex, nullptr))
            pPlayer->AttachWeapon(old);
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
            ++m_iClickFrame;   // frame clock for double-click detection (dt ~ 0 here)
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

        // 1) Onto a tower row → equip it there (placed or unplaced; reuse the arm
        //    path's one-tower-per-weapon check + assignment). Heal rows ignore it.
        for (int i = 0; i < m_iTowerCount; ++i)
            if (InRect(mx, my, m_TowerRect[i]))
            {
                if (m_eTowerRowSrc[i] == TowerRowSrc::Placed && !m_bTowerRowIsHeal[i])
                {
                    m_iEquipArmedWeaponId = iDragId; OnCycleTowerWeapon(i);
                }
                else if (m_eTowerRowSrc[i] == TowerRowSrc::AtkReserve)
                {
                    m_iEquipArmedWeaponId = iDragId; OnCycleReserveWeapon(m_iTowerRowReserve[i]);
                }
                return;
            }
        // 2) Onto the equipped strip → equip (only meaningful from inventory).
        const int iEquipCap = Player::GetMaxEquipSlots();
        for (int i = 0; i < kOwnedRows && i < iEquipCap; ++i)
            if (InRect(mx, my, m_OwnedRect[i]))
            {
                if (eSrc == DragSrc::Inventory) { pPlayer->EquipWeapon(iDragId); RebuildList(); }
                return;
            }
        // 3) Onto the inventory strip → unequip (only meaningful from equipped).
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
        // Inventory (idle) weapon icons — same per-weapon stat tooltip.
        if (wInfo.empty())
            for (int i = 0; i < kInvRows; ++i)
            {
                if (m_iInvIds[i] < 0 || !InRect(mx, my, m_InvRect[i])) continue;
                wInfo = weaponTip(m_iInvIds[i]);
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

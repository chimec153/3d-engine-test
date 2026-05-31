#include "TowerIntermissionUI.h"
#include "UI/Button.h"
#include "../Object/Player.h"
#include "../Object/Tower.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
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
    }

    TowerIntermissionUI::TowerIntermissionUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
        for (int i = 0; i < kBuyRows; ++i) { m_iBuyIds[i] = -1; m_eBuyKind[i] = BuyKind::Weapon; }
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
            y += S.fItemH + S.fGap;
        }

        // --- Your Weapons (drag sources) — a horizontal strip of icons ---
        m_pOwnedHeader = makeText("tower_shop_owned_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pOwnedHeader) m_pOwnedHeader->SetString(L"Your Weapons - drag onto a tower");
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
                    // No OnClick — drag is handled manually in HandleDrag().
                }
            }
            y += fIconH + S.fGap;
        }

        // --- Tower Loadout section (drop targets) ---
        m_pTowerHeader = makeText("tower_shop_tower_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pTowerHeader) m_pTowerHeader->SetString(L"Towers - drop a weapon here (or click to cycle)");
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
                m_pTowerButtons[i]->SetOnClick([this, idx]() { OnCycleTowerWeapon(idx); });
            }
            m_pTowerTexts[i] = makeText("tower_shop_tower_t" + std::to_string(i), y, S.fItemH, m_pItemFont, Engine::Text::HAlign::Center);
            y += S.fItemH + S.fGap;
        }

        // --- Unplaced Towers (weapon per bought-but-unplaced tower) ---
        // A horizontal icon strip (like Your Weapons). Leftmost = the next
        // tower placed (FIFO). Drop a weapon on a slot or click to cycle.
        m_pReserveHeader = makeText("tower_shop_reserve_h", y, S.fHeaderH, m_pItemFont, Engine::Text::HAlign::Left);
        if (m_pReserveHeader) m_pReserveHeader->SetString(L"Unplaced Towers (left = placed first) - drop a weapon or click to cycle");
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
        if (m_pTowerHeader)    m_pTowerHeader->Enable();
        if (m_pReserveHeader)  m_pReserveHeader->Enable();
        if (m_pStartButton)    m_pStartButton->Enable();
        if (m_pStartText)      m_pStartText->Enable();
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
        if (m_pTowerHeader)    m_pTowerHeader->Disable();
        if (m_pReserveHeader)  m_pReserveHeader->Disable();
        if (m_pStartButton)    m_pStartButton->Disable();
        if (m_pStartText)      m_pStartText->Disable();
        if (m_pDragGhost)      m_pDragGhost->Disable();
        m_bDragging = false;
        for (int i = 0; i < kOwnedRows; ++i)
            if (m_pOwnedIcons[i]) m_pOwnedIcons[i]->Disable();
        for (int i = 0; i < kBuyRows; ++i)
        {
            if (m_pBuyButtons[i]) m_pBuyButtons[i]->Disable();
            if (m_pBuyTexts[i])   m_pBuyTexts[i]->Disable();
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
        // (shown as a merge/level-up buy); the equip section is where you pick
        // among them.
        // The catalog mixes weapons and towers: every shop-available weapon plus
        // an attack-tower and a heal-tower entry, shuffled, first kBuyRows shown.
        // So towers roll in randomly alongside weapons (same as how a weapon may
        // or may not appear on any given shop open).
        struct Cand { BuyKind kind; int id; };
        std::vector<Cand> all;
        const std::vector<int> vecCrafted = WeaponDatabase::GetInst().AllCraftedLiveIds();
        for (int id : WeaponDatabase::GetInst().ShopWeaponIds())
        {
            if (std::find(vecCrafted.begin(), vecCrafted.end(), id) != vecCrafted.end())
                continue;   // skip session-crafted weapons
            all.push_back({ BuyKind::Weapon, id });
        }
        all.push_back({ BuyKind::Tower,     -1 });
        all.push_back({ BuyKind::HealTower, -1 });

        const int n = static_cast<int>(all.size());
        for (int i = 0; i < n && i < kBuyRows; ++i)
        {
            const int j = i + std::rand() % (n - i);
            std::swap(all[i], all[j]);
        }
        m_iBuyCount = (std::min)(kBuyRows, n);
        for (int i = 0; i < kBuyRows; ++i)
        {
            if (i < m_iBuyCount) { m_eBuyKind[i] = all[i].kind; m_iBuyIds[i] = all[i].id; }
            else                 { m_eBuyKind[i] = BuyKind::Weapon; m_iBuyIds[i] = -1; }
        }
    }

    void TowerIntermissionUI::RerollBuySlot(int iIndex)
    {
        if (iIndex < 0 || iIndex >= kBuyRows) return;

        // Same candidate pool RollCatalog draws from: shop weapons (minus
        // session-crafted) plus an attack-tower and a heal-tower entry.
        struct Cand { BuyKind kind; int id; };
        std::vector<Cand> all;
        const std::vector<int> vecCrafted = WeaponDatabase::GetInst().AllCraftedLiveIds();
        for (int id : WeaponDatabase::GetInst().ShopWeaponIds())
        {
            if (std::find(vecCrafted.begin(), vecCrafted.end(), id) != vecCrafted.end())
                continue;
            all.push_back({ BuyKind::Weapon, id });
        }
        all.push_back({ BuyKind::Tower,     -1 });
        all.push_back({ BuyKind::HealTower, -1 });

        // Prefer a pick that isn't already displayed in another slot, so the
        // replacement is a genuinely different item. Fall back to the full pool
        // if every candidate is already on screen (small pool).
        auto bShownElsewhere = [&](const Cand& c) -> bool
        {
            for (int i = 0; i < m_iBuyCount; ++i)
            {
                if (i == iIndex) continue;
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

        // --- Buy catalog rows (random weapons + towers) ---
        for (int i = 0; i < kBuyRows; ++i)
        {
            const bool bHas = i < m_iBuyCount;
            const int  id   = m_iBuyIds[i];
            if (!bHas)
            {
                if (m_pBuyButtons[i]) m_pBuyButtons[i]->Disable();
                if (m_pBuyTexts[i])   m_pBuyTexts[i]->Disable();
                syncOutline(i, L"", false);
                continue;
            }
            // Tower rows: a flat colour button + price (blue=attack, green=heal).
            if (m_eBuyKind[i] == BuyKind::Tower || m_eBuyKind[i] == BuyKind::HealTower)
            {
                const bool bHeal  = (m_eBuyKind[i] == BuyKind::HealTower);
                const int  iPrice = bHeal ? kHealTowerPrice : kTowerPrice;
                if (m_pBuyButtons[i])
                {
                    m_pBuyButtons[i]->SetTexture(EnsureSolidTexture(
                        bHeal ? "tower_shop_buyheal_bg" : "tower_shop_buytower_bg",
                        bHeal ? 0x1E7A3C : 0x18558A));
                    m_pBuyButtons[i]->Enable();
                }
                const std::wstring wTowerLabel =
                    (bHeal ? L"Heal Tower  $" : L"Tower  $") + std::to_wstring(iPrice);
                if (m_pBuyTexts[i])
                {
                    m_pBuyTexts[i]->SetColor((iMoney >= iPrice) ? 0xFFFFFFFFu : 0x808080FFu);
                    m_pBuyTexts[i]->SetString(wTowerLabel);
                    m_pBuyTexts[i]->Enable();
                }
                syncOutline(i, wTowerLabel, true);
                continue;
            }
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const std::wstring wName = pDef ? ToW(pDef->strName) : L"Weapon";
            const int  iOwnedLevel = pPlayer ? pPlayer->GetOwnedWeaponLevel(id) : 0;
            const bool bOwned = iOwnedLevel > 0;

            if (m_pBuyButtons[i])
            {
                const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
                m_pBuyButtons[i]->SetTexture(
                    EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
                m_pBuyButtons[i]->Enable();
            }
            if (m_pBuyTexts[i])
            {
                std::wstring wLabel;
                unsigned int uTextColor;
                if (bOwned)
                {
                    // Already owned → buying the duplicate MERGES it (levels up
                    // the existing slot). Allowed even when the loadout is full.
                    wLabel = wName + L"  Lv." + std::to_wstring(iOwnedLevel) +
                             L" merge $" + std::to_wstring(kWeaponPrice);
                    uTextColor = (iMoney >= kWeaponPrice) ? 0x60C0FFFFu : 0x808080FFu;
                }
                else if (bLoadoutFull)
                {
                    wLabel = wName + L"  (FULL)";
                    uTextColor = 0x808080FFu;
                }
                else
                {
                    wLabel = wName + L"  $" + std::to_wstring(kWeaponPrice);
                    uTextColor = (iMoney >= kWeaponPrice) ? 0x60FF60FFu : 0x808080FFu;
                }
                m_pBuyTexts[i]->SetColor(uTextColor);
                m_pBuyTexts[i]->SetString(wLabel);
                m_pBuyTexts[i]->Enable();
                syncOutline(i, wLabel, true);
            }
        }

        // --- Your Weapons (drag-source icons) ---
        const std::vector<int> vecOwned = pPlayer ? pPlayer->GetOwnedWeaponIds() : std::vector<int>{};
        for (int i = 0; i < kOwnedRows; ++i)
        {
            const bool bHas = i < static_cast<int>(vecOwned.size());
            const int  id   = bHas ? vecOwned[i] : -1;
            m_iOwnedIds[i] = id;
            if (!m_pOwnedIcons[i]) continue;
            if (!bHas) { m_pOwnedIcons[i]->Disable(); continue; }
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
            m_pOwnedIcons[i]->SetTexture(EnsureSolidTexture("tower_shop_w_" + std::to_string(id), uColor));
            m_pOwnedIcons[i]->Enable();
        }

        // --- Tower Loadout rows (each placed tower's own weapon) ---
        // Gather the placed towers from the scene (stable while frozen).
        std::vector<std::shared_ptr<Tower>> vecTowers;
        {
            auto* pOwner = GetGameObjectOwner();
            Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
            std::shared_ptr<Engine::Layer> pLayer =
                pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
            if (pLayer)
                for (const auto& p : pLayer->GetGameObjectList())
                    if (p && p->IsActive() && p->GetTag() == "Tower")
                        if (auto pT = std::dynamic_pointer_cast<Tower>(p))
                            vecTowers.push_back(pT);
        }
        m_iTowerCount = (std::min)(kTowerRows, static_cast<int>(vecTowers.size()));
        for (int i = 0; i < kTowerRows; ++i)
        {
            const bool bHas = i < m_iTowerCount;
            if (!bHas)
            {
                m_pTowerRowRefs[i].reset();
                if (m_pTowerButtons[i]) m_pTowerButtons[i]->Disable();
                if (m_pTowerTexts[i])   m_pTowerTexts[i]->Disable();
                continue;
            }
            auto pT = vecTowers[i];
            m_pTowerRowRefs[i] = pT;
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
                m_pTowerTexts[i]->SetString(L"Tower " + std::to_wstring(i + 1) + L": " + wName);
                m_pTowerTexts[i]->Enable();
            }
        }

        // --- Unplaced Towers (reserve weapon per bought-but-unplaced tower) ---
        // Each icon's colour is the weapon that reserve tower will fire; a raw
        // -1 (unconfigured) resolves to the current default for display.
        const int iReserve  = TowerManager::GetInst().ReserveCount();
        const int iDefaultW = TowerManager::GetInst().CurrentWeaponId();
        m_iReserveCount = (std::min)(kReserveRows, iReserve);
        for (int i = 0; i < kReserveRows; ++i)
        {
            if (!m_pReserveIcons[i]) continue;
            if (i >= m_iReserveCount) { m_pReserveIcons[i]->Disable(); continue; }
            int wid = TowerManager::GetInst().ReserveWeaponRaw(i);
            if (wid < 0) wid = iDefaultW;
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(wid);
            const unsigned int uColor = pDef ? pDef->uColorRGB : 0x606060;
            m_pReserveIcons[i]->SetTexture(
                EnsureSolidTexture("tower_shop_w_" + std::to_string(wid), uColor));
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
        if (iIndex < 0 || iIndex >= m_iBuyCount) return;

        // Tower catalog rows: buy adds to the tower inventory (a new unplaced
        // tower; its weapon defaults to the current one, configurable in the
        // Unplaced Towers strip).
        if (m_eBuyKind[iIndex] == BuyKind::Tower)
        {
            if (!Wallet::GetInst().TrySpend(kTowerPrice)) return;
            TowerManager::GetInst().AddTower();
            RerollBuySlot(iIndex);
            RebuildList();
            return;
        }
        if (m_eBuyKind[iIndex] == BuyKind::HealTower)
        {
            if (!Wallet::GetInst().TrySpend(kHealTowerPrice)) return;
            TowerManager::GetInst().AddHealTower();
            RerollBuySlot(iIndex);
            RebuildList();
            return;
        }

        const int id = m_iBuyIds[iIndex];
        if (id < 0) return;

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;
        // Already owned → this purchase MERGES the duplicate (levels up the
        // existing slot). A new weapon needs a free loadout slot; a merge does
        // not, so the loadout-full guard only applies to new weapons.
        const bool bOwned = pPlayer->GetOwnedWeaponLevel(id) > 0;
        if (!bOwned && pPlayer->GetWeaponSlotCount() >= Player::GetMaxWeaponSlots()) return;   // loadout full
        if (!Wallet::GetInst().TrySpend(kWeaponPrice)) return;   // can't afford

        // Owns it now (or levels it up). Add it to the player's loadout and make
        // it the default weapon for newly placed towers (assign it to specific
        // towers in the Tower Loadout section below). AddOrLevelUpWeapon handles
        // the merge (level-up + evolution) when the weapon is already owned.
        pPlayer->AddOrLevelUpWeapon(id);
        TowerManager::GetInst().SetCurrentWeaponId(id);
        RerollBuySlot(iIndex);
        RebuildList();
    }

    void TowerIntermissionUI::OnCycleTowerWeapon(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (iIndex < 0 || iIndex >= m_iTowerCount) return;
        auto pTower = m_pTowerRowRefs[iIndex].lock();
        if (!pTower) return;
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Cycle this tower's weapon to the player's next owned weapon.
        const std::vector<int> vecOwned = pPlayer->GetOwnedWeaponIds();
        if (vecOwned.empty()) return;
        const int cur = pTower->GetWeaponId();
        int idx = -1;
        for (int i = 0; i < static_cast<int>(vecOwned.size()); ++i)
            if (vecOwned[i] == cur) { idx = i; break; }
        const int next = (idx + 1) % static_cast<int>(vecOwned.size());
        pTower->SetWeaponId(vecOwned[next]);
        RebuildList();
    }

    void TowerIntermissionUI::OnCycleReserveWeapon(int iIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        if (iIndex < 0 || iIndex >= m_iReserveCount) return;
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Cycle this unplaced tower's weapon to the player's next owned weapon.
        const std::vector<int> vecOwned = pPlayer->GetOwnedWeaponIds();
        if (vecOwned.empty()) return;
        int cur = TowerManager::GetInst().ReserveWeaponRaw(iIndex);
        if (cur < 0) cur = TowerManager::GetInst().CurrentWeaponId();
        int idx = -1;
        for (int i = 0; i < static_cast<int>(vecOwned.size()); ++i)
            if (vecOwned[i] == cur) { idx = i; break; }
        const int next = (idx + 1) % static_cast<int>(vecOwned.size());
        TowerManager::GetInst().SetReserveWeapon(iIndex, vecOwned[next]);
        RebuildList();
    }

    void TowerIntermissionUI::OnStart()
    {
        if (GameStateManager::GetInst().GetState() != GameState::Intermission) return;
        Hide();
        m_bShownLocal = false;
        if (m_fnStart) m_fnStart();
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
            HandleDrag();
    }

    void TowerIntermissionUI::HandleDrag()
    {
        auto* pInput = Engine::CInput::GetInst();
        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());
        using MOUSE = Engine::CInput::MOUSE_TYPE;

        // Start a drag: left-press began this frame over an owned-weapon icon.
        if (!m_bDragging && pInput->IsMouseButtonDown(MOUSE::LEFT))
        {
            for (int i = 0; i < kOwnedRows; ++i)
            {
                if (m_iOwnedIds[i] < 0) continue;
                if (!InRect(mx, my, m_OwnedRect[i])) continue;
                m_bDragging     = true;
                m_iDragWeaponId = m_iOwnedIds[i];
                if (m_pDragGhost)
                {
                    // Tint the ghost to the dragged weapon's colour.
                    const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iDragWeaponId);
                    const unsigned int uColor = pDef ? pDef->uColorRGB : 0x808080;
                    m_pDragGhost->SetTexture(TowerIntermissionUI_detail::EnsureSolidTexture(
                        "tower_shop_w_" + std::to_string(m_iDragWeaponId), uColor));
                    m_pDragGhost->Enable();
                }
                break;
            }
        }

        if (!m_bDragging) return;

        // While held: the ghost follows the cursor (centred on it).
        const float fGhost = m_OwnedRect[0].h;   // square, row height
        if (m_pDragGhost)
            m_pDragGhost->SetRect(mx - fGhost * 0.5f, my - fGhost * 0.5f, fGhost, fGhost);

        // Release: drop onto a placed-tower row or an unplaced (reserve) slot to
        // equip; otherwise just cancel.
        if (pInput->IsMouseButtonUp(MOUSE::LEFT))
        {
            bool bDropped = false;
            for (int i = 0; i < m_iTowerCount; ++i)
            {
                if (!InRect(mx, my, m_TowerRect[i])) continue;
                if (auto pTower = m_pTowerRowRefs[i].lock())
                    pTower->SetWeaponId(m_iDragWeaponId);
                bDropped = true;
                break;
            }
            if (!bDropped)
                for (int i = 0; i < m_iReserveCount; ++i)
                {
                    if (!InRect(mx, my, m_ReserveRect[i])) continue;
                    TowerManager::GetInst().SetReserveWeapon(i, m_iDragWeaponId);
                    break;
                }
            m_bDragging     = false;
            m_iDragWeaponId = -1;
            if (m_pDragGhost) m_pDragGhost->Disable();
            RebuildList();
        }
    }

    std::shared_ptr<Engine::Component> TowerIntermissionUI::Clone()
    {
        return std::make_shared<TowerIntermissionUI>(*this);
    }
}

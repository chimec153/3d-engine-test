#include "TowerHUD.h"
#include "../Object/Tower.h"
#include "../Object/TowerManager.h"
#include "../Object/TowerData.h"
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

        for (int i = 0; i < kSlotCount; ++i)
        {
            const float fX = fLeft + i * (fSlotSize + fSlotGap);

            std::string tagBox = "btn_tower_hud_" + std::to_string(i);
            m_pBoxes[i] = CreateComponent<Engine::Button>(tagBox);
            if (m_pBoxes[i])
            {
                m_pBoxes[i]->SetRect(fX, fTop, fSlotSize, fSlotSize);
                m_pBoxes[i]->SetTexture(TowerHUD_detail::EmptySlotTexture());
            }

            // Weapon-colour square, bottom-left corner. Created after the box so
            // it draws on top of it; the texts (created next) draw on top again.
            std::string tagDot = "btn_tower_hud_wdot_" + std::to_string(i);
            m_pWeaponDots[i] = CreateComponent<Engine::Button>(tagDot);
            if (m_pWeaponDots[i])
            {
                const float fDot = fSlotSize * 0.30f;
                const float fPad = fSlotSize * 0.06f;
                m_pWeaponDots[i]->SetRect(fX + fPad, fTop + fSlotSize - fDot - fPad, fDot, fDot);
                m_pWeaponDots[i]->SetTexture(TowerHUD_detail::EmptySlotTexture());
                m_pWeaponDots[i]->Disable();   // shown only when the slot is filled
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
                const float fInsetY = fSlotSize * 0.08f;
                m_pNameTexts[i]->SetRect(
                    fX + fInsetX,
                    fTop + fInsetY,
                    fSlotSize - 2.f * fInsetX,
                    fSlotSize * 0.70f);
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
        return true;
    }

    void TowerHUD::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);
        using namespace TowerHUD_detail;

        // Gather the owned attack towers: placed ones first (live "Tower" scene
        // objects), then the reserve entries (unplaced or on cooldown). State:
        // 0 placed, 1 ready, 2 down. Capped at the slot row width.
        struct Ent { int iTowerId; int iWeaponId; int iLevel; int iState; };
        Ent ents[kSlotCount];
        int n = 0;

        auto* pOwner = GetGameObjectOwner();
        Engine::Scene* pScene = pOwner ? pOwner->GetScene() : nullptr;
        std::shared_ptr<Engine::Layer> pLayer =
            pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
        if (pLayer)
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (n >= kSlotCount) break;
                if (!p || !p->IsActive() || p->GetTag() != "Tower") continue;
                auto pT = std::static_pointer_cast<Tower>(p);
                ents[n++] = { pT->GetTowerDefId(), pT->GetWeaponId(), pT->GetLevel(), 0 };
            }

        auto& mgr = TowerManager::GetInst();
        const int iReserve = mgr.ReserveCount();
        for (int i = 0; i < iReserve && n < kSlotCount; ++i)
        {
            int wid = mgr.ReserveWeaponRaw(i);
            if (wid < 0) wid = mgr.CurrentWeaponId();
            ents[n++] = { mgr.ReserveTowerId(i), wid, mgr.ReserveLevel(i), mgr.ReserveDown(i) ? 2 : 1 };
        }

        for (int i = 0; i < kSlotCount; ++i)
        {
            const bool bHas    = i < n;
            const int iTowerId = bHas ? ents[i].iTowerId  : -1;
            const int iId      = bHas ? ents[i].iWeaponId : -1;
            const int iLevel   = bHas ? ents[i].iLevel    : -1;
            const int iState   = bHas ? ents[i].iState    : -1;

            if (iTowerId == m_iLastTowerIds[i] && iId == m_iLastIds[i] &&
                iLevel == m_iLastLevels[i] && iState == m_iLastStates[i])
                continue;
            m_iLastTowerIds[i] = iTowerId;
            m_iLastIds[i]      = iId;
            m_iLastLevels[i]   = iLevel;
            m_iLastStates[i]   = iState;

            if (!bHas)
            {
                if (m_pBoxes[i])      m_pBoxes[i]->SetTexture(EmptySlotTexture());
                if (m_pWeaponDots[i]) m_pWeaponDots[i]->Disable();
                if (m_pNameTexts[i])  m_pNameTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])   m_pLvlTexts[i]->SetString(L"");
                continue;
            }

            // Tower TYPE -> box colour + centred name. id < 0 is the default
            // attack type (resolve to the first attack row for its name).
            const TowerDef* pTD = (iTowerId >= 0)
                ? TowerDatabase::GetInst().Get(iTowerId)
                : TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
            const std::wstring wsType = pTD
                ? std::wstring(pTD->strName.begin(), pTD->strName.end())
                : std::wstring(L"Tower");

            // Equipped WEAPON -> bottom-left square colour.
            const WeaponDef* pWDef = (iId >= 0) ? WeaponDatabase::GetInst().Get(iId) : nullptr;
            const unsigned int uWeaponColor = pWDef ? pWDef->uColorRGB : 0x606060u;
            if (m_pWeaponDots[i])
            {
                m_pWeaponDots[i]->SetTexture(WeaponDotTexture(iId, uWeaponColor));
                m_pWeaponDots[i]->Enable();
            }

            if (iState == 2)   // destroyed this round - on cooldown
            {
                if (m_pBoxes[i]) m_pBoxes[i]->SetTexture(DownSlotTexture());
                if (m_pNameTexts[i]) { m_pNameTexts[i]->SetColor(0x909090FFu); m_pNameTexts[i]->SetString(wsType); }
                if (m_pLvlTexts[i])  { m_pLvlTexts[i]->SetColor(0xFF6060FFu);  m_pLvlTexts[i]->SetString(L"CD"); }
            }
            else
            {
                if (m_pBoxes[i]) m_pBoxes[i]->SetTexture(TypeBgTexture(iTowerId, iState));
                if (m_pNameTexts[i]) { m_pNameTexts[i]->SetColor(0xFFFFFFFFu); m_pNameTexts[i]->SetString(wsType); }
                if (m_pLvlTexts[i])
                {
                    // Ready (not yet deployed) slots get a cool blue tint on the
                    // level badge; placed ones stay white.
                    m_pLvlTexts[i]->SetColor(iState == 1 ? 0xC0E0FFFFu : 0xFFFFFFFFu);
                    m_pLvlTexts[i]->SetString(L"Lv." + std::to_wstring(iLevel));
                }
            }
        }
    }

    std::shared_ptr<Engine::Component> TowerHUD::Clone()
    {
        return std::make_shared<TowerHUD>(*this);
    }
}

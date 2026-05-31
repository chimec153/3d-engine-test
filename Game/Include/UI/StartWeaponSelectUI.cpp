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
    }

    void StartWeaponSelectUI::BuildList()
    {
        // The shop-available weapon catalogue (v2 + crafted), first kMaxItems
        // in deterministic order -- same pool the between-round shop sells from.
        const std::vector<int> vecIds = WeaponDatabase::GetInst().ShopWeaponIds();
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
    }

    std::shared_ptr<Engine::Component> StartWeaponSelectUI::Clone()
    {
        return std::make_shared<StartWeaponSelectUI>(*this);
    }
}

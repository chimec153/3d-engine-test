#include "LevelUpChoices.h"
#include "Button.h"
#include "../Object/Player.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Types.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace Client
{
    namespace LevelUpChoices_detail
    {
        // Target card layout, in *screen pixels*:
        //   - card height ~55% of the window height
        //   - card aspect ratio 2:3 (portrait)
        //   - 3 cards side-by-side, gap ~3% of window width between them
        // Pixel → NDC at runtime so the layout looks the same on 16:9,
        // 16:10 and ultra-wide displays.
        constexpr float kCardHeightFrac  = 0.55f;
        constexpr float kSpacingFracW    = 0.03f;
        constexpr float kCardAspectWH    = 2.f / 3.f;     // w/h

        struct CardLayout
        {
            float fCardW;     // NDC width
            float fCardH;     // NDC height
            float fSpacing;   // NDC spacing between cards
            float fLeftX;     // NDC left edge of card 0
            float fBaseY;     // NDC bottom edge of all cards
            int   iCardPxW;   // Card pixel width — feeds Text rasteriser
            int   iCardPxH;   // Card pixel height
        };

        CardLayout ComputeCardLayout()
        {
            const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
            const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

            const float fCardPxH = kCardHeightFrac * fScreenH;
            const float fCardPxW = fCardPxH * kCardAspectWH;
            const float fSpacePx = kSpacingFracW * fScreenW;

            CardLayout L;
            L.fCardW   = 2.f * fCardPxW / fScreenW;
            L.fCardH   = 2.f * fCardPxH / fScreenH;
            L.fSpacing = 2.f * fSpacePx / fScreenW;
            const float fTotalW = 3.f * L.fCardW + 2.f * L.fSpacing;
            L.fLeftX   = -0.5f * fTotalW;
            L.fBaseY   = -0.5f * L.fCardH;
            L.iCardPxW = (std::max)(64, static_cast<int>(fCardPxW));
            L.iCardPxH = (std::max)(64, static_cast<int>(fCardPxH));
            return L;
        }

        // ABGR memory layout: bytes are stored R, G, B, A so the 32-bit
        // literal reads A<<24 | B<<16 | G<<8 | R.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        // 1x1 single-colour card background. The dark rim from the old
        // GDI path is gone — the colour alone reads as a clean panel,
        // and bilinear sampling on a 1x1 texture is identical regardless
        // of card size so the GPU upscale is free.
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

        std::shared_ptr<Engine::Texture> EnsureWeaponBgTexture(int iWeaponId, unsigned int uRGB)
        {
            std::string strTag = "weapon_bg_" + std::to_string(iWeaponId);
            return EnsureSolidTexture(strTag, uRGB);
        }
        std::shared_ptr<Engine::Texture> EnsureBlankCardTexture()
        {
            return EnsureSolidTexture("weapon_bg_blank", 0x222222);
        }
    }

    LevelUpChoices::LevelUpChoices()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool LevelUpChoices::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        const auto L = LevelUpChoices_detail::ComputeCardLayout();

        // Fonts are sized off the card pixel height so they read at the
        // same on-screen size regardless of window resolution. Once
        // built they're keyed by tag in FontManager — a second
        // LevelUpChoices instance would reuse the same Font objects.
        const float fNameSize = 14.f;// (std::max)(14.f, L.iCardPxH * 0.10f);
        const float fLvlSize = 18.f;// (std::max)(18.f, L.iCardPxH * 0.14f);
        m_pNameFont = Engine::FontManager::GetInst()->CreateFont(
            "card_name", L"Arial", fNameSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pLvlFont  = Engine::FontManager::GetInst()->CreateFont(
            "card_lvl",  L"Arial", fLvlSize,  DWRITE_FONT_WEIGHT_BOLD);

        // Per-card text-region pixel sizes. Match the geometry of the
        // overlay Buttons below (40% top band for the name, 30% bottom
        // band for the level).
        const int iNameTexH = static_cast<int>(L.iCardPxH * 0.40f);
        const int iLvlTexH  = static_cast<int>(L.iCardPxH * 0.30f);

        // Name-band Y centres at ~75% up the card (NDC +Y is up); level
        // band sits at ~17% up. Heights match the texture heights so
        // the bands cover the right pixel real-estate.
        const float fNameBandH = L.fCardH * 0.40f;
        const float fLvlBandH  = L.fCardH * 0.30f;
        const float fNameBandY = L.fBaseY + L.fCardH * 0.55f;   // bottom edge
        const float fLvlBandY  = L.fBaseY + L.fCardH * 0.05f;

        for (int i = 0; i < 3; ++i)
        {
            const float fX = L.fLeftX + i * (L.fCardW + L.fSpacing);

            // Background button — coloured panel, owns the click handler.
            std::string tagBg = "button_card_bg_" + std::to_string(i);
            m_pBgButtons[i] = CreateComponent<Button>(tagBg);
            if (m_pBgButtons[i])
            {
                m_pBgButtons[i]->SetRect(fX, L.fBaseY, L.fCardW, L.fCardH);
                m_pBgButtons[i]->SetTexture(LevelUpChoices_detail::EnsureBlankCardTexture());
                const int idx = i;
                m_pBgButtons[i]->SetOnClick([this, idx]() { OnPick(idx); });
            }

            // Text components — created *after* the Button so they
            // appear later in m_ChildList and therefore render on top
            // (UIRenderer's PreDraw is dispatched in child-list order,
            // and RenderManager keeps that order in its UI custom-
            // render queue).
            std::string tagName = "text_card_name_" + std::to_string(i);
            m_pNameTexts[i] = CreateComponent<Engine::Text>(tagName);
            if (m_pNameTexts[i])
            {
                m_pNameTexts[i]->SetFont(m_pNameFont);
                m_pNameTexts[i]->SetTextureSize(20, 20);
                m_pNameTexts[i]->SetColor(0xFFFFFFFFu);
                m_pNameTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pNameTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pNameTexts[i]->SetRect(0.f, 0.f, 20 / 1280.f, 20 / 720.f);
            }

            std::string tagLvl = "text_card_lvl_" + std::to_string(i);
            m_pLvlTexts[i] = CreateComponent<Engine::Text>(tagLvl);
            if (m_pLvlTexts[i])
            {
                m_pLvlTexts[i]->SetFont(m_pLvlFont);
                m_pLvlTexts[i]->SetTextureSize(L.iCardPxW, iLvlTexH);
                m_pLvlTexts[i]->SetColor(0xFFFFFFFFu);
                m_pLvlTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pLvlTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pLvlTexts[i]->SetRect(fX, fLvlBandY, L.fCardW, fLvlBandH);
            }
        }

        Hide();
        return true;
    }

    void LevelUpChoices::Show()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (m_pBgButtons[i])  m_pBgButtons[i]->Enable();
            if (m_pNameTexts[i])  m_pNameTexts[i]->Enable();
            if (m_pLvlTexts[i])   m_pLvlTexts[i]->Enable();
        }
    }

    void LevelUpChoices::Hide()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (m_pBgButtons[i])  m_pBgButtons[i]->Disable();
            if (m_pNameTexts[i])  m_pNameTexts[i]->Disable();
            if (m_pLvlTexts[i])   m_pLvlTexts[i]->Disable();
        }
    }

    void LevelUpChoices::RollCards()
    {
        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        const auto& vecAll = WeaponDatabase::GetInst().All();
        if (vecAll.empty())
        {
            for (int i = 0; i < 3; ++i)
            {
                m_iCardWeaponIds[i] = -1;
                if (m_pBgButtons[i])
                    m_pBgButtons[i]->SetTexture(LevelUpChoices_detail::EnsureBlankCardTexture());
                if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(L"");
            }
            return;
        }

        const auto vecOwned = pPlayer->GetOwnedWeaponIds();
        const bool bSlotsFull = pPlayer->GetWeaponSlotCount() >= Player::GetMaxWeaponSlots();

        std::vector<int> vecUnowned;
        std::vector<int> vecOwnedIds = vecOwned;
        for (const auto& def : vecAll)
        {
            const bool bAlreadyOwn =
                std::find(vecOwned.begin(), vecOwned.end(), def.iId) != vecOwned.end();
            if (!bAlreadyOwn && !bSlotsFull) vecUnowned.push_back(def.iId);
        }

        std::vector<int> vecPool;
        vecPool.reserve(vecUnowned.size() + vecOwnedIds.size());
        for (int id : vecUnowned)  vecPool.push_back(id);
        for (int id : vecOwnedIds) vecPool.push_back(id);

        for (size_t i = 0; i < vecPool.size() && i < 3; ++i)
        {
            size_t j = i + static_cast<size_t>(std::rand()) % (vecPool.size() - i);
            std::swap(vecPool[i], vecPool[j]);
        }

        for (int i = 0; i < 3; ++i)
        {
            const int id = (i < static_cast<int>(vecPool.size())) ? vecPool[i] : -1;
            m_iCardWeaponIds[i] = id;
            if (id < 0)
            {
                if (m_pBgButtons[i])
                    m_pBgButtons[i]->SetTexture(LevelUpChoices_detail::EnsureBlankCardTexture());
                if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(L"");
                continue;
            }

            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(id);
            if (!pDef) continue;
            const int iCurrent = pPlayer->GetOwnedWeaponLevel(id);
            const int iDisplayLevel = iCurrent > 0 ? iCurrent + 1 : 1;

            if (m_pBgButtons[i])
                m_pBgButtons[i]->SetTexture(
                    LevelUpChoices_detail::EnsureWeaponBgTexture(id, pDef->uColorRGB));

            // Text components rebuild their own textures inside Update
            // when SetString invalidates the cache, then push the new
            // texture into their internal UIRenderer. No bridging from
            // LevelUpChoices needed.
            if (m_pNameTexts[i])
                m_pNameTexts[i]->SetString(std::wstring(
                    pDef->strName.begin(), pDef->strName.end()));
            if (m_pLvlTexts[i])
                m_pLvlTexts[i]->SetString(L"Lv. " + std::to_wstring(iDisplayLevel));
        }
    }

    void LevelUpChoices::OnPick(int iCardIndex)
    {
        if (!m_bShown) return;
        if (iCardIndex < 0 || iCardIndex >= 3) return;

        if (auto pPlayer = m_pTarget.lock())
        {
            const int iWeaponId = m_iCardWeaponIds[iCardIndex];
            pPlayer->ConsumeLevelUp(iWeaponId);
        }

        Hide();
        Engine::Window::GetInst()->GetTimer()->Resume();
        m_bShown = false;
    }

    void LevelUpChoices::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        const bool bPending = pPlayer->HasPendingLevelUp();
        if (bPending && !m_bShown)
        {
            RollCards();
            Show();
            Engine::Window::GetInst()->GetTimer()->Stop();
            m_bShown = true;
        }
    }

    std::shared_ptr<Engine::Component> LevelUpChoices::Clone()
    {
        return std::make_shared<LevelUpChoices>(*this);
    }
}

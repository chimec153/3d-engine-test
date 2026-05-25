#include "WeaponHUD.h"
#include "../Object/Player.h"
#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "UI/Button.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Core/Window.h"
#include "Types.h"
#include <algorithm>
#include <string>

namespace Client
{
    namespace WeaponHUD_detail
    {
        // Layout, applied to the current window size at Init. Six square
        // slot boxes laid out horizontally along the top-left corner;
        // a small gap between each so the boxes read as a row of icons.
        constexpr float kLeftFrac     = 0.020f;
        constexpr float kTopFrac      = 0.025f;
        constexpr float kSlotSizeFrac = 0.085f; // fraction of window height (square)
        constexpr float kSlotGapFrac  = 0.008f; // fraction of window width

        // ABGR memory layout matches the texture-upload byte order: R, G,
        // B, A. The 1x1 init data is a single uint32 so the bytes must
        // be ordered R-first.
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

        std::shared_ptr<Engine::Texture> WeaponBgTexture(int iWeaponId, unsigned int uRGB)
        {
            std::string strTag = "weapon_hud_bg_" + std::to_string(iWeaponId);
            return EnsureSolidTexture(strTag, uRGB);
        }

        // Faintly translucent dark grey — empty slots read as "no
        // weapon" without dominating the HUD.
        std::shared_ptr<Engine::Texture> EmptySlotTexture()
        {
            return EnsureSolidTexture("weapon_hud_bg_empty", 0x222222, 0x80);
        }
    }

    WeaponHUD::WeaponHUD()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
        for (int i = 0; i < kSlotCount; ++i)
        {
            m_iLastIds[i]    = -1;
            m_iLastLevels[i] = -1;
        }
    }

    bool WeaponHUD::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        const float fSlotSize = (std::max)(48.f, fScreenH * WeaponHUD_detail::kSlotSizeFrac);
        const float fSlotGap  = (std::max)( 4.f, fScreenW * WeaponHUD_detail::kSlotGapFrac);
        const float fLeft     = fScreenW * WeaponHUD_detail::kLeftFrac;
        const float fTop      = fScreenH * WeaponHUD_detail::kTopFrac;

        // Name font sized so a ~8-char word fits on one line and longer
        // names break across two. Level font is a touch larger but still
        // small relative to the box so it reads as a corner badge.
        const float fNameSize = (std::max)(11.f, fSlotSize * 0.16f);
        const float fLvlSize  = (std::max)(10.f, fSlotSize * 0.18f);

        m_pNameFont = Engine::FontManager::GetInst()->CreateFont(
            "weapon_hud_name", L"Arial", fNameSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pLvlFont = Engine::FontManager::GetInst()->CreateFont(
            "weapon_hud_lvl",  L"Arial", fLvlSize,  DWRITE_FONT_WEIGHT_BOLD);

        for (int i = 0; i < kSlotCount; ++i)
        {
            const float fX = fLeft + i * (fSlotSize + fSlotGap);

            // Background box — Button gives us SetTexture; the (unused)
            // OnClick stays null so a click is harmlessly absorbed.
            std::string tagBox = "btn_weapon_hud_" + std::to_string(i);
            m_pBoxes[i] = CreateComponent<Engine::Button>(tagBox);
            if (m_pBoxes[i])
            {
                m_pBoxes[i]->SetRect(fX, fTop, fSlotSize, fSlotSize);
                m_pBoxes[i]->SetTexture(WeaponHUD_detail::EmptySlotTexture());
            }

            // Name text — fills most of the box; DirectWrite wraps to
            // two lines when the name is too long for one. Vertically
            // biased upward so the bottom-right Lv badge has clear
            // space underneath.
            std::string tagName = "text_weapon_hud_name_" + std::to_string(i);
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

            // Level badge — bottom-right corner. Right + Bottom aligned
            // inside a small sub-rect, so the digits hug the corner
            // regardless of how many characters wide "Lv.N" is.
            std::string tagLvl = "text_weapon_hud_lvl_" + std::to_string(i);
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

    void WeaponHUD::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        const auto vecIds = pPlayer->GetOwnedWeaponIds();
        for (int i = 0; i < kSlotCount; ++i)
        {
            const int iId    = (i < static_cast<int>(vecIds.size())) ? vecIds[i] : -1;
            const int iLevel = (iId >= 0) ? pPlayer->GetOwnedWeaponLevel(iId) : -1;

            if (iId == m_iLastIds[i] && iLevel == m_iLastLevels[i]) continue;
            m_iLastIds[i]    = iId;
            m_iLastLevels[i] = iLevel;

            const WeaponDef* pDef = (iId >= 0) ? WeaponDatabase::GetInst().Get(iId) : nullptr;
            if (!pDef)
            {
                if (m_pBoxes[i])     m_pBoxes[i]->SetTexture(WeaponHUD_detail::EmptySlotTexture());
                if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(L"");
                continue;
            }

            if (m_pBoxes[i])
                m_pBoxes[i]->SetTexture(
                    WeaponHUD_detail::WeaponBgTexture(iId, pDef->uColorRGB));

            std::wstring wsName(pDef->strName.begin(), pDef->strName.end());
            if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(wsName);
            if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(L"Lv." + std::to_wstring(iLevel));
        }
    }

    std::shared_ptr<Engine::Component> WeaponHUD::Clone()
    {
        return std::make_shared<WeaponHUD>(*this);
    }
}

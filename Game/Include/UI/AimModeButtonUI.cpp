#include "AimModeButtonUI.h"
#include "../Object/Player.h"
#include "UI/Button.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Types.h"
#include <algorithm>
#include <string>

namespace Client
{
    namespace AimModeButtonUI_detail
    {
        // Named namespace (not anonymous): Game shares the unity/jumbo-build
        // hazard where per-file anonymous helpers collide across TUs.
        constexpr unsigned int kColorOn  = 0x2E7D32;   // green  — target mode on
        constexpr unsigned int kColorOff = 0x455A64;   // slate  — target mode off

        // ABGR memory layout (bytes R,G,B,A) — matches PauseMenuUI.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Engine::Texture> EnsureSolidTexture(unsigned int uRGB, unsigned int uAlpha)
        {
            std::string strTag = "aimmode_solid_" + std::to_string(uRGB) + "_" + std::to_string(uAlpha);
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
    }

    AimModeButtonUI::AimModeButtonUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool AimModeButtonUI::Init()
    {
        using namespace AimModeButtonUI_detail;
        if (!Engine::UIControl::Init()) return false;

        const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        m_pFont = Engine::FontManager::GetInst()->CreateFont(
            "aimmode_btn", L"Malgun Gothic", (std::max)(16.f, H * 0.024f), DWRITE_FONT_WEIGHT_BOLD);
        m_pHintFont = Engine::FontManager::GetInst()->CreateFont(
            "aimmode_hint", L"Malgun Gothic", (std::max)(12.f, H * 0.017f), DWRITE_FONT_WEIGHT_NORMAL);

        // Square button + a key-hint caption beneath it, bottom-left, lifted
        // ABOVE the HP/XP gauges so neither covers the health bar. The gauges
        // occupy y in [0.9325H, 0.975H], x in [0.025W, 0.225W] (UIObjects.cpp);
        // keep the button AND its caption above the XP bar's top (0.9325H).
        const float fSide  = (std::max)(64.f, H * 0.085f);   // square side
        const float fGap   = (std::max)(10.f, H * 0.012f);   // clearance above XP bar
        const float fHintH = (std::max)(16.f, H * 0.022f);   // caption strip height
        const float fBtnX  = W * 0.025f;
        const float fBtnY  = H * 0.9325f - fGap - fHintH - fSide;

        m_pButton = CreateComponent<Engine::Button>("aimmode_btn_bg");
        if (m_pButton)
        {
            m_pButton->SetRect(fBtnX, fBtnY, fSide, fSide);
            m_pButton->SetTexture(EnsureSolidTexture(kColorOff, 0xE0));
            m_pButton->SetOnClick([this]
            {
                // Same effect as pressing Ctrl. Update() re-syncs the label.
                if (auto p = m_pTarget.lock()) p->ToggleMouseAim();
            });
        }

        m_pLabel = CreateComponent<Engine::Text>("aimmode_btn_txt");
        if (m_pLabel)
        {
            m_pLabel->SetFont(m_pFont);
            m_pLabel->SetColor(0xFFFFFFFFu);
            m_pLabel->SetHAlign(Engine::Text::HAlign::Center);
            m_pLabel->SetVAlign(Engine::Text::VAlign::Center);
            m_pLabel->SetRect(fBtnX, fBtnY, fSide, fSide);
            m_pLabel->SetString(L"타겟\nOFF");
        }

        // Key-press hint directly below the button.
        m_pHint = CreateComponent<Engine::Text>("aimmode_btn_hint");
        if (m_pHint)
        {
            m_pHint->SetFont(m_pHintFont);
            m_pHint->SetColor(0xCCCCCCFFu);
            m_pHint->SetHAlign(Engine::Text::HAlign::Center);
            m_pHint->SetVAlign(Engine::Text::VAlign::Center);
            m_pHint->SetRect(fBtnX, fBtnY + fSide, fSide, fHintH);
            m_pHint->SetString(L"[Ctrl]");
        }

        return true;
    }

    void AimModeButtonUI::ApplyState(bool bOn)
    {
        using namespace AimModeButtonUI_detail;
        if (m_pButton)
            m_pButton->SetTexture(EnsureSolidTexture(bOn ? kColorOn : kColorOff, 0xE0));
        if (m_pLabel)
            m_pLabel->SetString(bOn ? L"타겟\nON" : L"타겟\nOFF");
    }

    void AimModeButtonUI::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Re-apply label/colour only on an actual change so Engine::Text's baked
        // glyph cache isn't re-rendered every frame (same guard as WeaponHUD).
        const int iState = pPlayer->IsMouseAim() ? 1 : 0;
        if (iState != m_iLastState)
        {
            m_iLastState = iState;
            ApplyState(iState != 0);
        }
    }

    std::shared_ptr<Engine::Component> AimModeButtonUI::Clone()
    {
        return std::make_shared<AimModeButtonUI>(*this);
    }
}

#include "PauseMenuUI.h"
#include "UI/Button.h"
#include "../Object/GameStateManager.h"
#include "../Scene/StartScene.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Input/Input.h"
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Scene/SceneManager.h"
#include "Types.h"
#include <algorithm>
#include <string>

namespace Client
{
    namespace PauseMenuUI_detail
    {
        constexpr unsigned int kBackdropColor = 0x101015;  // dark dim
        constexpr unsigned int kBackdropAlpha = 0xC0;       // ~75% — game shows through
        constexpr unsigned int kButtonColor   = 0x37474F;   // slate

        // ABGR memory layout (bytes R,G,B,A) — matches GameOverUI.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Engine::Texture> EnsureSolidTexture(unsigned int uRGB, unsigned int uAlpha)
        {
            std::string strTag = "pause_solid_" + std::to_string(uRGB) + "_" + std::to_string(uAlpha);
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

    PauseMenuUI::PauseMenuUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool PauseMenuUI::Init()
    {
        using namespace PauseMenuUI_detail;
        if (!Engine::UIControl::Init()) return false;

        // Register Escape so IsKey(DOWN, ...) can read its press edge. Guard
        // against re-adding on a scene reload (AddKey doesn't de-dupe).
        if (!Engine::CInput::GetInst()->FindKey(DIK_ESCAPE))
            Engine::CInput::GetInst()->AddKey(DIK_ESCAPE);

        const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        m_pTitleFont = Engine::FontManager::GetInst()->CreateFont(
            "pause_title", L"Malgun Gothic", (std::max)(40.f, H * 0.09f), DWRITE_FONT_WEIGHT_BOLD);
        m_pBtnFont = Engine::FontManager::GetInst()->CreateFont(
            "pause_btn", L"Malgun Gothic", (std::max)(20.f, H * 0.030f), DWRITE_FONT_WEIGHT_BOLD);

        // Dimmed backdrop (created first so it draws behind the text/buttons).
        // Semi-transparent so the frozen game stays visible underneath.
        m_pBackdrop = CreateComponent<Engine::Button>("pause_backdrop");
        if (m_pBackdrop)
        {
            m_pBackdrop->SetRect(0.f, 0.f, W, H);
            m_pBackdrop->SetTexture(EnsureSolidTexture(kBackdropColor, kBackdropAlpha));
        }

        // "일시정지" caption.
        m_pTitleText = CreateComponent<Engine::Text>("pause_title_txt");
        if (m_pTitleText)
        {
            m_pTitleText->SetFont(m_pTitleFont);
            m_pTitleText->SetColor(0xFFFFFFFFu);
            m_pTitleText->SetHAlign(Engine::Text::HAlign::Center);
            m_pTitleText->SetVAlign(Engine::Text::VAlign::Center);
            m_pTitleText->SetRect(0.f, H * 0.24f, W, H * 0.16f);
            m_pTitleText->SetString(L"일시정지");
        }

        // Two stacked buttons, centred.
        const float fBtnW = (std::max)(260.f, W * 0.26f);
        const float fBtnH = (std::max)(54.f,  H * 0.09f);
        const float fBtnX = (W - fBtnW) * 0.5f;
        const float fGap  = fBtnH * 0.35f;
        const float fResumeY = H * 0.50f;
        const float fQuitY   = fResumeY + fBtnH + fGap;

        m_pResumeBtn = CreateComponent<Engine::Button>("pause_resume_btn");
        if (m_pResumeBtn)
        {
            m_pResumeBtn->SetRect(fBtnX, fResumeY, fBtnW, fBtnH);
            m_pResumeBtn->SetTexture(EnsureSolidTexture(kButtonColor, 0xFF));
            m_pResumeBtn->SetOnClick([this] { OnResume(); });
        }
        m_pResumeText = CreateComponent<Engine::Text>("pause_resume_txt");
        if (m_pResumeText)
        {
            m_pResumeText->SetFont(m_pBtnFont);
            m_pResumeText->SetColor(0xFFFFFFFFu);
            m_pResumeText->SetHAlign(Engine::Text::HAlign::Center);
            m_pResumeText->SetVAlign(Engine::Text::VAlign::Center);
            m_pResumeText->SetRect(fBtnX, fResumeY, fBtnW, fBtnH);
            m_pResumeText->SetString(L"이어하기");
        }

        m_pQuitBtn = CreateComponent<Engine::Button>("pause_quit_btn");
        if (m_pQuitBtn)
        {
            m_pQuitBtn->SetRect(fBtnX, fQuitY, fBtnW, fBtnH);
            m_pQuitBtn->SetTexture(EnsureSolidTexture(kButtonColor, 0xFF));
            m_pQuitBtn->SetOnClick([this] { OnQuit(); });
        }
        m_pQuitText = CreateComponent<Engine::Text>("pause_quit_txt");
        if (m_pQuitText)
        {
            m_pQuitText->SetFont(m_pBtnFont);
            m_pQuitText->SetColor(0xFFFFFFFFu);
            m_pQuitText->SetHAlign(Engine::Text::HAlign::Center);
            m_pQuitText->SetVAlign(Engine::Text::VAlign::Center);
            m_pQuitText->SetRect(fBtnX, fQuitY, fBtnW, fBtnH);
            m_pQuitText->SetString(L"종료하기");
        }

        Hide();
        return true;
    }

    void PauseMenuUI::Show()
    {
        if (m_pBackdrop)   m_pBackdrop->Enable();
        if (m_pTitleText)  m_pTitleText->Enable();
        if (m_pResumeBtn)  m_pResumeBtn->Enable();
        if (m_pResumeText) m_pResumeText->Enable();
        if (m_pQuitBtn)    m_pQuitBtn->Enable();
        if (m_pQuitText)   m_pQuitText->Enable();
    }

    void PauseMenuUI::Hide()
    {
        if (m_pBackdrop)   m_pBackdrop->Disable();
        if (m_pTitleText)  m_pTitleText->Disable();
        if (m_pResumeBtn)  m_pResumeBtn->Disable();
        if (m_pResumeText) m_pResumeText->Disable();
        if (m_pQuitBtn)    m_pQuitBtn->Disable();
        if (m_pQuitText)   m_pQuitText->Disable();
    }

    void PauseMenuUI::OnResume()
    {
        Hide();
        GameStateManager::GetInst().ExitModal();
    }

    void PauseMenuUI::OnQuit()
    {
        // Resume the timer before leaving so the next game isn't frozen, then
        // swap back to the start screen (same exit as the game-over button).
        Hide();
        GameStateManager::GetInst().ExitModal();
        Engine::SceneManager::GetInst()->CreateScene<Client::StartScene>();
    }

    void PauseMenuUI::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        // ESC toggles the pause menu — but only between Playing and Paused.
        // While a LevelUp / GameOver modal owns the screen, ignore it.
        if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_ESCAPE))
        {
            auto& gsm = GameStateManager::GetInst();
            if (gsm.GetState() == GameState::Playing)
            {
                Show();
                gsm.EnterModal(GameState::Paused);
            }
            else if (gsm.GetState() == GameState::Paused)
            {
                OnResume();
            }
        }
    }

    std::shared_ptr<Engine::Component> PauseMenuUI::Clone()
    {
        return std::make_shared<PauseMenuUI>(*this);
    }
}

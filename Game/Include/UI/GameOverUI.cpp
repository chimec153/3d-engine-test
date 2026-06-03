#include "GameOverUI.h"
#include "UI/Button.h"
#include "../Object/Player.h"
#include "../Object/GameStateManager.h"
#include "../Scene/StartScene.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Core/Window.h"
#include "Render/RenderManager.h"   // clear the low-HP overlay on game over
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Scene/SceneManager.h"
#include "Types.h"
#include <algorithm>
#include <string>

namespace Client
{
    namespace GameOverUI_detail
    {
        constexpr unsigned int kBackdropColor = 0x101010;  // opaque dark cover
        constexpr unsigned int kButtonColor   = 0x37474F;  // slate

        // ABGR memory layout (bytes R,G,B,A) — matches LevelUpChoices.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Engine::Texture> EnsureSolidTexture(unsigned int uRGB)
        {
            std::string strTag = "gameover_solid_" + std::to_string(uRGB);
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

    GameOverUI::GameOverUI()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool GameOverUI::Init()
    {
        using namespace GameOverUI_detail;
        if (!Engine::UIControl::Init()) return false;

        const float W = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float H = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        // Malgun Gothic so the Korean captions render with real glyphs.
        m_pTitleFont = Engine::FontManager::GetInst()->CreateFont(
            "gameover_title", L"Malgun Gothic", (std::max)(40.f, H * 0.10f), DWRITE_FONT_WEIGHT_BOLD);
        m_pBtnFont = Engine::FontManager::GetInst()->CreateFont(
            "gameover_btn", L"Malgun Gothic", (std::max)(20.f, H * 0.030f), DWRITE_FONT_WEIGHT_BOLD);

        // Full-screen dark backdrop (created first so it draws behind the
        // text/button). No click handler — it just covers the frozen game.
        m_pBackdrop = CreateComponent<Engine::Button>("gameover_backdrop");
        if (m_pBackdrop)
        {
            m_pBackdrop->SetRect(0.f, 0.f, W, H);
            m_pBackdrop->SetTexture(EnsureSolidTexture(kBackdropColor));
        }

        // "게임 오버" caption.
        m_pTitleText = CreateComponent<Engine::Text>("gameover_title_txt");
        if (m_pTitleText)
        {
            m_pTitleText->SetFont(m_pTitleFont);
            m_pTitleText->SetColor(0xEF5350FFu);   // red (0xRRGGBBAA)
            m_pTitleText->SetHAlign(Engine::Text::HAlign::Center);
            m_pTitleText->SetVAlign(Engine::Text::VAlign::Center);
            m_pTitleText->SetRect(0.f, H * 0.28f, W, H * 0.18f);
            m_pTitleText->SetString(L"게임 오버");
        }

        // "메인 화면으로" button.
        const float fBtnW = (std::max)(260.f, W * 0.26f);
        const float fBtnH = (std::max)(54.f,  H * 0.10f);
        const float fBtnX = (W - fBtnW) * 0.5f;
        const float fBtnY = H * 0.56f;
        m_pButton = CreateComponent<Engine::Button>("gameover_btn");
        if (m_pButton)
        {
            m_pButton->SetRect(fBtnX, fBtnY, fBtnW, fBtnH);
            m_pButton->SetTexture(EnsureSolidTexture(kButtonColor));
            m_pButton->SetOnClick([this] { OnButton(); });
        }
        m_pButtonText = CreateComponent<Engine::Text>("gameover_btn_txt");
        if (m_pButtonText)
        {
            m_pButtonText->SetFont(m_pBtnFont);
            m_pButtonText->SetColor(0xFFFFFFFFu);
            m_pButtonText->SetHAlign(Engine::Text::HAlign::Center);
            m_pButtonText->SetVAlign(Engine::Text::VAlign::Center);
            m_pButtonText->SetRect(fBtnX, fBtnY, fBtnW, fBtnH);
            m_pButtonText->SetString(L"메인 화면으로");
        }

        Hide();
        return true;
    }

    void GameOverUI::Show()
    {
        if (m_pBackdrop)   m_pBackdrop->Enable();
        if (m_pTitleText)  m_pTitleText->Enable();
        if (m_pButton)     m_pButton->Enable();
        if (m_pButtonText) m_pButtonText->Enable();
    }

    void GameOverUI::Hide()
    {
        if (m_pBackdrop)   m_pBackdrop->Disable();
        if (m_pTitleText)  m_pTitleText->Disable();
        if (m_pButton)     m_pButton->Disable();
        if (m_pButtonText) m_pButtonText->Disable();
    }

    void GameOverUI::OnButton()
    {
        // Latch dismissal first so Update won't re-enter GameOver after the
        // ExitModal below (player is still dead, IsPlaying flips back true).
        m_bDismissed = true;
        // Resume the timer before leaving so the next game isn't frozen,
        // then swap back to the start screen.
        GameStateManager::GetInst().ExitModal();
        Engine::SceneManager::GetInst()->CreateScene<Client::StartScene>();
    }

    void GameOverUI::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        // Already dismissed (button pressed, scene change pending) — don't
        // re-arm the modal.
        if (m_bDismissed) return;

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Trigger once: the EnterModal flips the state off Playing, so this
        // branch won't re-fire. The button (polled regardless of the pause)
        // is what leaves the screen.
        if (pPlayer->IsDead() && GameStateManager::GetInst().IsPlaying())
        {
            Show();
            GameStateManager::GetInst().EnterModal(GameState::GameOver);
            // The low-HP vignette is pushed every frame by Player::Update; once
            // the game-over modal stops the timer the player stops updating, so
            // the last (full-strength) overlay would stay stuck on screen.
            // Clear it explicitly so the near-death HUD turns off with the game.
            Engine::RenderManager::GetInst()->SetLowHp(0.f);
        }
    }

    std::shared_ptr<Engine::Component> GameOverUI::Clone()
    {
        return std::make_shared<GameOverUI>(*this);
    }
}

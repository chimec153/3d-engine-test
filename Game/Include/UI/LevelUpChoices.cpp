#include "LevelUpChoices.h"
#include "UI/Button.h"
#include "../Object/Player.h"
#include "../Object/LevelUpDatabase.h"
#include "../Object/GameStateManager.h"
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
#include <cwchar>
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
        // UIControl now takes pixel coords directly, so we no longer
        // round-trip through NDC.
        constexpr float kCardHeightFrac  = 0.55f;
        constexpr float kSpacingFracW    = 0.03f;
        constexpr float kCardAspectWH    = 2.f / 3.f;     // w/h

        struct CardLayout
        {
            float fCardW;     // Card pixel width
            float fCardH;     // Card pixel height
            float fSpacing;   // Pixel gap between adjacent cards
            float fLeftX;     // Top-left pixel X of card 0
            float fTopY;      // Top-left pixel Y of every card
        };

        CardLayout ComputeCardLayout()
        {
            const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
            const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

            CardLayout L;
            L.fCardH   = kCardHeightFrac * fScreenH;
            L.fCardW   = L.fCardH * kCardAspectWH;
            L.fSpacing = kSpacingFracW * fScreenW;

            // Centre the row of cards on screen.
            const float fTotalW = 3.f * L.fCardW + 2.f * L.fSpacing;
            L.fLeftX = (fScreenW - fTotalW) * 0.5f;
            L.fTopY  = (fScreenH - L.fCardH) * 0.5f;
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

        std::shared_ptr<Engine::Texture> EnsureBlankCardTexture()
        {
            return EnsureSolidTexture("levelup_blank", 0x222222);
        }

        // Widen an ASCII card string (loaded from levelups.csv) for the Text UI.
        std::wstring ToW(const std::string& s) { return std::wstring(s.begin(), s.end()); }

        // Build a card's effect line from its template + amount, so the number
        // lives only in the `amount` column (no duplication). Tokens:
        //   {a} -> raw amount (e.g. 12, 0.5, 3)
        //   {p} -> percent of amount (amount*100 as an integer, with %).
        std::wstring FormatEffect(const std::string& strTemplate, float fAmount)
        {
            wchar_t raw[32]; std::swprintf(raw, 32, L"%g", fAmount);
            const int iPct = static_cast<int>(fAmount * 100.f + (fAmount < 0.f ? -0.5f : 0.5f));
            wchar_t pct[32]; std::swprintf(pct, 32, L"%d%%", iPct);

            std::wstring s = ToW(strTemplate);
            auto replaceAll = [](std::wstring& str, const std::wstring& from, const std::wstring& to)
            {
                size_t pos = 0;
                while ((pos = str.find(from, pos)) != std::wstring::npos)
                {
                    str.replace(pos, from.size(), to);
                    pos += to.size();
                }
            };
            replaceAll(s, L"{a}", raw);
            replaceAll(s, L"{p}", pct);
            return s;
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

        // Font pixel sizes scale with the card height so the text reads
        // at the same proportional size on any resolution. Once built
        // they're keyed by tag in FontManager — a second LevelUpChoices
        // instance would reuse the same Font objects.
        const float fNameSize = (std::max)(14.f, L.fCardH * 0.10f);
        const float fLvlSize  = (std::max)(18.f, L.fCardH * 0.14f);
        m_pNameFont = Engine::FontManager::GetInst()->CreateFont(
            "card_name", L"Arial", fNameSize, DWRITE_FONT_WEIGHT_BOLD);
        m_pLvlFont  = Engine::FontManager::GetInst()->CreateFont(
            "card_lvl",  L"Arial", fLvlSize,  DWRITE_FONT_WEIGHT_BOLD);

        // Per-card text bands, all in pixels relative to the card's
        // top-left (Y grows downward, matching the window pixel space
        // UIControl now uses):
        //   - name band: top ~5% to ~45% of card height
        //   - level band: bottom ~70% to ~95% of card height
        const float fNameBandH = L.fCardH * 0.40f;
        const float fLvlBandH  = L.fCardH * 0.30f;
        const float fNameBandDY = L.fCardH * 0.05f;
        const float fLvlBandDY  = L.fCardH * 0.65f;

        for (int i = 0; i < 3; ++i)
        {
            const float fX = L.fLeftX + i * (L.fCardW + L.fSpacing);

            // Background button — coloured panel, owns the click handler.
            std::string tagBg = "button_card_bg_" + std::to_string(i);
            m_pBgButtons[i] = CreateComponent<Engine::Button>(tagBg);
            if (m_pBgButtons[i])
            {
                m_pBgButtons[i]->SetRect(fX, L.fTopY, L.fCardW, L.fCardH);
                m_pBgButtons[i]->SetTexture(LevelUpChoices_detail::EnsureBlankCardTexture());
                const int idx = i;
                m_pBgButtons[i]->SetOnClick([this, idx]() { OnPick(idx); });
            }

            // Text components — created after the Button so RenderUI's
            // custom-render queue draws them on top of the panel.
            std::string tagName = "text_card_name_" + std::to_string(i);
            m_pNameTexts[i] = CreateComponent<Engine::Text>(tagName);
            if (m_pNameTexts[i])
            {
                m_pNameTexts[i]->SetFont(m_pNameFont);
                m_pNameTexts[i]->SetColor(0xFFFFFFFFu);
                m_pNameTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pNameTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pNameTexts[i]->SetRect(fX, L.fTopY + fNameBandDY, L.fCardW, fNameBandH);
            }

            std::string tagLvl = "text_card_lvl_" + std::to_string(i);
            m_pLvlTexts[i] = CreateComponent<Engine::Text>(tagLvl);
            if (m_pLvlTexts[i])
            {
                m_pLvlTexts[i]->SetFont(m_pLvlFont);
                m_pLvlTexts[i]->SetColor(0xFFFFFFFFu);
                m_pLvlTexts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_pLvlTexts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_pLvlTexts[i]->SetRect(fX, L.fTopY + fLvlBandDY, L.fCardW, fLvlBandH);
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
        using namespace LevelUpChoices_detail;
        // Draw 3 distinct cards from the data-driven catalogue by weighted draw
        // without replacement — each row's `weight` (lower for percent options)
        // is its relative odds. std::rand matches the rest of the game.
        const auto& cards = LevelUpDatabase::GetInst().All();
        const int n = static_cast<int>(cards.size());
        std::vector<int> remaining;
        remaining.reserve(n);
        for (int s = 0; s < n; ++s) remaining.push_back(s);

        std::vector<int> pool;   // chosen catalogue indices, in draw order
        for (int pick = 0; pick < 3 && !remaining.empty(); ++pick)
        {
            int iTotal = 0;
            for (int s : remaining) iTotal += cards[s].weight;
            int r = (iTotal > 0) ? (std::rand() % iTotal) : 0;
            int sel = 0;
            for (size_t k = 0; k < remaining.size(); ++k)
            {
                r -= cards[remaining[k]].weight;
                if (r < 0) { sel = static_cast<int>(k); break; }
            }
            pool.push_back(remaining[sel]);
            remaining.erase(remaining.begin() + sel);
        }

        for (int i = 0; i < 3; ++i)
        {
            const int idx = (i < static_cast<int>(pool.size())) ? pool[i] : -1;
            m_iCardStats[i] = idx;
            if (idx < 0)
            {
                if (m_pBgButtons[i]) m_pBgButtons[i]->SetTexture(EnsureBlankCardTexture());
                if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(L"");
                if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(L"");
                continue;
            }

            const LevelUpDef& d = cards[idx];
            if (m_pBgButtons[i])
                m_pBgButtons[i]->SetTexture(EnsureSolidTexture("levelup_" + d.key, d.colorRGB));
            if (m_pNameTexts[i]) m_pNameTexts[i]->SetString(ToW(d.name));
            if (m_pLvlTexts[i])  m_pLvlTexts[i]->SetString(FormatEffect(d.effect, d.amount));
        }
    }

    void LevelUpChoices::OnPick(int iCardIndex)
    {
        if (GameStateManager::GetInst().GetState() != GameState::LevelUpModal) return;
        if (iCardIndex < 0 || iCardIndex >= 3) return;

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        const int idx = m_iCardStats[iCardIndex];
        const auto& cards = LevelUpDatabase::GetInst().All();
        if (idx < 0 || idx >= static_cast<int>(cards.size())) return;
        pPlayer->ApplyStatUpgrade(cards[idx].key, cards[idx].amount);

        // Sequential drain: a multi-level pickup queues several picks. If more
        // remain, re-roll and stay in the modal; only exit when the queue is
        // empty (ApplyStatUpgrade decrements the pending counter).
        if (pPlayer->HasPendingLevelUp())
        {
            RollCards();
        }
        else
        {
            Hide();
            GameStateManager::GetInst().ExitModal();
        }
    }

    void LevelUpChoices::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer) return;

        // Open the modal once when the queue first appears — re-entry is
        // guarded by GameStateManager's state, not a local boolean, so
        // a clone or accidental hide can't leave the state stuck.
        if (pPlayer->HasPendingLevelUp() &&
            GameStateManager::GetInst().IsPlaying())
        {
            RollCards();
            Show();
            GameStateManager::GetInst().EnterModal(GameState::LevelUpModal);
        }
    }

    std::shared_ptr<Engine::Component> LevelUpChoices::Clone()
    {
        return std::make_shared<LevelUpChoices>(*this);
    }
}

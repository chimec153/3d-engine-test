#include "StartMenu.h"
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
    namespace StartMenu_detail
    {
        // ABGR memory layout (R,G,B,A bytes) — same packing as LevelUpChoices.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha = 0xFF)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        // 1x1 solid-colour panel texture, cached by tag. Bilinear upscale to
        // any button size is free for a 1x1 source.
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

    StartMenu::StartMenu(std::vector<MenuItem> items)
        : Engine::UIControl()
        , m_items(std::move(items))
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool StartMenu::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        const int iCount = static_cast<int>(m_items.size());
        if (iCount <= 0) return true;

        const float fScreenW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float fScreenH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        // Vertical stack centred on screen.
        const float fBtnW = (std::max)(240.f, fScreenW * 0.30f);
        const float fBtnH = (std::max)(48.f,  fScreenH * 0.11f);
        const float fGap  = fScreenH * 0.035f;
        const float fTotalH = iCount * fBtnH + (iCount - 1) * fGap;
        const float fLeftX  = (fScreenW - fBtnW) * 0.5f;
        const float fTopY   = (fScreenH - fTotalH) * 0.5f;

        // Malgun Gothic — the standard Windows Korean UI font, so the
        // Korean menu captions render with real glyphs instead of relying
        // on DirectWrite fallback from a Latin family.
        const float fFontSize = (std::max)(18.f, fBtnH * 0.42f);
        m_font = Engine::FontManager::GetInst()->CreateFont(
            "menu_item", L"Malgun Gothic", fFontSize, DWRITE_FONT_WEIGHT_BOLD);

        m_buttons.resize(iCount);
        m_texts.resize(iCount);

        for (int i = 0; i < iCount; ++i)
        {
            const float fY = fTopY + i * (fBtnH + fGap);
            const MenuItem& item = m_items[i];

            // Panel button (owns the click handler).
            std::string tagBtn = "menu_btn_" + std::to_string(i);
            m_buttons[i] = CreateComponent<Engine::Button>(tagBtn);
            if (m_buttons[i])
            {
                m_buttons[i]->SetRect(fLeftX, fY, fBtnW, fBtnH);
                std::string tagTex = "menu_btn_tex_" + std::to_string(item.colorRGB);
                m_buttons[i]->SetTexture(
                    StartMenu_detail::EnsureSolidTexture(tagTex, item.colorRGB));
                std::function<void()> fn = item.onClick;
                m_buttons[i]->SetOnClick([fn]() { if (fn) fn(); });
            }

            // Caption text, centred over the panel. Created after the button
            // so RenderUI's custom-render queue draws it on top.
            std::string tagTxt = "menu_txt_" + std::to_string(i);
            m_texts[i] = CreateComponent<Engine::Text>(tagTxt);
            if (m_texts[i])
            {
                m_texts[i]->SetFont(m_font);
                m_texts[i]->SetColor(0xFFFFFFFFu);
                m_texts[i]->SetHAlign(Engine::Text::HAlign::Center);
                m_texts[i]->SetVAlign(Engine::Text::VAlign::Center);
                m_texts[i]->SetRect(fLeftX, fY, fBtnW, fBtnH);
                m_texts[i]->SetString(item.label);
            }
        }

        return true;
    }

    std::shared_ptr<Engine::Component> StartMenu::Clone()
    {
        return std::make_shared<StartMenu>(*this);
    }
}

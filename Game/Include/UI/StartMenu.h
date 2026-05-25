#pragma once

#include "UI/UIControl.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Engine
{
    class Button;
    class Text;
    class Font;
}

namespace Client
{
    // One labelled, clickable menu entry: a coloured panel button plus a
    // centred text caption, with a click handler.
    struct MenuItem
    {
        std::wstring          label;
        unsigned int          colorRGB = 0x333333;   // button panel colour (0xRRGGBB)
        std::function<void()> onClick;
    };

    // A vertical stack of MenuItem buttons centred on screen. Reused for any
    // full-screen menu (start menu, weapon-combo placeholder). Items are taken
    // by the constructor because GameObject::AddComponent calls Init()
    // immediately — they must already be present when Init builds the widgets.
    // Each entry mirrors LevelUpChoices' Button + Text composition.
    class StartMenu : public Engine::UIControl
    {
    public:
        StartMenu() = default;
        explicit StartMenu(std::vector<MenuItem> items);
        StartMenu(const StartMenu& other) = default;
        virtual ~StartMenu() override = default;

        virtual bool Init() override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        std::vector<MenuItem> m_items;
        std::vector<std::shared_ptr<Engine::Button>> m_buttons;
        std::vector<std::shared_ptr<Engine::Text>>   m_texts;
        std::shared_ptr<Engine::Font> m_font;
    };
}

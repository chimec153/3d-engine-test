#pragma once

#include "GameObject/GameObject.h"

namespace Client
{
    // GameObject wrappers around UI-only Components so they show up in
    // the Editor's actor palette (GameObjectFactory::ListAll). Each
    // wrapper's Init attaches the matching UIControl-derived Component
    // and applies the same anchor / colour setup GameScene uses, so a
    // Spawn from the Editor produces a visually identical widget.
    //
    // Player-binding (Gauge::SetTarget, LevelUpChoices::SetTarget) is
    // still done by GameScene at scene-init time; the Editor path
    // creates the widgets without a target so the bars render at zero
    // ratio until something wires them up.

    class HPBarObject : public Engine::GameObject
    {
    public:
        HPBarObject() = default;
        virtual bool Init() override;
    };

    class XPBarObject : public Engine::GameObject
    {
    public:
        XPBarObject() = default;
        virtual bool Init() override;
    };

    class LevelUpChoicesObject : public Engine::GameObject
    {
    public:
        LevelUpChoicesObject() = default;
        virtual bool Init() override;
    };
}

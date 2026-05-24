#include "UIObjects.h"
#include "Core/ObjectFactory.h"
#include "UI/Gauge.h"
#include "../UI/LevelUpChoices.h"

// Registration tokens — name strings match GameScene's CreateGameObject
// calls so an Editor-spawned widget and a GameScene-spawned widget land
// in the same Layer slot.
REGISTER_GAMEOBJECT(Client::HPBarObject,          HPBar)
REGISTER_GAMEOBJECT(Client::XPBarObject,          XPBar)
REGISTER_GAMEOBJECT(Client::LevelUpChoicesObject, LevelUpChoices)

namespace Client
{
    bool HPBarObject::Init()
    {
        if (!Engine::GameObject::Init()) return false;
        if (auto pHP = AddComponent<Engine::Gauge>("hpbar"))
        {
            // Mirrors GameScene::Init — red fill on dark-grey track,
            // anchored 2.5% in from the bottom-left, growing up-right.
            pHP->SetColors(0xFF303030, 0xFF2030E0);
            pHP->SetRectByAnchorFrac(
                Engine::Vector2{ 0.025f, 0.975f },
                Engine::Vector2{ 0.f,    1.f    },
                Engine::Vector2{ 0.2f,   0.025f });
        }
        return true;
    }

    bool XPBarObject::Init()
    {
        if (!Engine::GameObject::Init()) return false;
        if (auto pXP = AddComponent<Engine::Gauge>("xpbar"))
        {
            // Yellow fill on darker grey, sitting just above the HP bar.
            pXP->SetColors(0xFF202020, 0xFF20D0E0);
            pXP->SetRectByAnchorFrac(
                Engine::Vector2{ 0.025f, 0.945f },
                Engine::Vector2{ 0.f,    1.f    },
                Engine::Vector2{ 0.2f,   0.0125f });
        }
        return true;
    }

    bool LevelUpChoicesObject::Init()
    {
        if (!Engine::GameObject::Init()) return false;
        AddComponent<LevelUpChoices>("levelup");
        return true;
    }
}

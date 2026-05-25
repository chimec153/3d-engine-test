#pragma once

#include "Scene/Scene.h"
#include "Core/Macro.h"

namespace Client
{
    // Placeholder weapon-combination screen reached from the start menu.
    // Empty for now (camera + a single "back to menu" button); the actual
    // combination UI/logic lands in a later pass. GAME_DLL to match the
    // other Client scenes.
    class GAME_DLL WeaponComboScene : public Engine::Scene
    {
    public:
        WeaponComboScene();
        virtual ~WeaponComboScene() override;

        virtual bool Init() override;
    };
}

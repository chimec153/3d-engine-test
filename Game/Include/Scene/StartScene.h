#pragma once

#include "Scene/Scene.h"
#include "Core/Macro.h"
namespace Client
{
    // GAME_DLL so Client's main.cpp (CreateScene<StartScene> → new StartScene)
    // can link the ctor/vtable out of Game.dll, same as GameScene.
    class GAME_DLL StartScene :
        public Engine::Scene
    {
    public:
        StartScene();
        virtual ~StartScene() override;

    public:
        virtual bool Init() override;
    };
}
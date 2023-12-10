#pragma once

#include "Scene/Scene.h"
namespace Engine
{
    class Collider;
}

namespace Client
{
    class GameScene :
        public Engine::Scene
    {
    public:
        GameScene();
        virtual ~GameScene() override = default;
            
    public:
        virtual bool Init() override;
    };
}
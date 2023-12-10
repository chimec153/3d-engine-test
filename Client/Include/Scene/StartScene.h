#pragma once

#include "Scene/Scene.h"
namespace Client
{
    class StartScene :
        public Engine::Scene
    {
    public:
        StartScene();
        virtual ~StartScene() override;

    public:
        virtual bool Init() override;
    };
}
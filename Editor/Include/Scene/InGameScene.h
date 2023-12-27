#pragma once
#include "Scene\Scene.h"

namespace Editor
{
    class InGameScene :
        public Engine::Scene
    {
    public:
        InGameScene();
        virtual ~InGameScene() = default;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
    };

}
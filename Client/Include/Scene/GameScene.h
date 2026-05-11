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
        bool LoadSequences();
        bool CreateTexture();
        bool CreateSounds();
        bool CreateMesh();

    public:
        bool CreateMonster();

    public:
        virtual bool Init() override;
        virtual void Update(float dt) override;
        virtual void Draw() override;
    };
}
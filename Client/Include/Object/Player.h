#pragma once

#include "Bindable/Drawable.h"

namespace Engine
{
    class Terrain;
}

namespace Client
{
    class Player :
        public Engine::Drawable
    {
    public:
        Player();
        virtual ~Player() override = default;

    private:
        float m_fSpeed;
        std::shared_ptr<Engine::Camera> m_pCamera;
        std::shared_ptr<Engine::Terrain> m_pTerrain;

    public:
        virtual void Start() override;
        virtual bool Init() override;
        virtual void Input(float fDeltaTime) override;
        virtual void Update(float fDeltaTime) override;
    };

}
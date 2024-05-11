#pragma once
#include "Bindable\Drawable.h"
namespace Client
{
    class Bullet :
        public Engine::Drawable
    {
    public:
        Bullet();
        virtual ~Bullet() override = default;

    private:
        float m_fSpeed;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
    };
}
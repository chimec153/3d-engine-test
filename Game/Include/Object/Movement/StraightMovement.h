#pragma once
#include "IBulletMovement.h"

namespace Client
{
    // Constant forward velocity along local +Y (the engine convention after
    // Bullet's RX=-π/2 + RY=yaw chain). Stateless.
    class StraightMovement final : public IBulletMovement
    {
    public:
        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) override;
    };
}

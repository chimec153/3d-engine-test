#pragma once
#include "IBulletMovement.h"

namespace Client
{
    // Forward + perpendicular cosine sway. Phase accumulator lives here so
    // the sway resumes deterministically frame to frame.
    class SpiralMovement final : public IBulletMovement
    {
    public:
        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) override;

    private:
        float m_fPhase = 0.f;
    };
}

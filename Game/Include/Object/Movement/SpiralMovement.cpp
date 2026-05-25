#include "SpiralMovement.h"
#include "Bindable/Transform.h"
#include <cmath>

namespace Client
{
    void SpiralMovement::Update(Engine::Transform& transform,
                                float fSpeed, float fDeltaTime)
    {
        m_fPhase += fDeltaTime;

        const float fDist = fDeltaTime * fSpeed;
        transform.AddPosition(transform.GetAxis(Engine::AXIS_TYPE::Y) * fDist);

        // Perpendicular sway along local X (world-right after the RX/RY chain).
        constexpr float kAmp  = 1.5f;
        constexpr float kFreq = 6.f;
        const float fSide = std::cos(m_fPhase * kFreq) * kAmp * fDeltaTime;
        transform.AddPosition(transform.GetAxis(Engine::AXIS_TYPE::X) * fSide);
    }
}

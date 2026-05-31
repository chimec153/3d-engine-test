#include "HomingMovement.h"
#include "EnemyTargeting.h"
#include "Bindable/Transform.h"
#include "GameObject/GameObject.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    namespace
    {
        // Max heading change per second while homing (rad/sec). High enough to
        // track a moving enemy, low enough that the missile visibly curves
        // toward the target instead of snapping straight at it.
        constexpr float kHomingTurnRate = 5.0f;
    }

    void HomingMovement::Update(Engine::Transform& transform,
                                float fSpeed, float fDeltaTime)
    {
        const Engine::Vector3 vPos = transform.GetPosition();

        // Re-acquire only when the held target is gone (dead / despawned) so
        // the missile commits to one enemy per its aim mode instead of
        // re-picking every frame (which would make LowestHP / Random thrash).
        auto pTarget = m_wpTarget.lock();
        if (!pTarget || !pTarget->IsActive())
        {
            pTarget = FindTargetEnemy(vPos, m_eAimMode);
            m_wpTarget = pTarget;
        }

        // Steer the heading toward the target by the capped turn rate. Yaw
        // convention matches Player aim: forward = (-sin yaw, 0, -cos yaw), so
        // the yaw facing a direction is atan2(-dir.x, -dir.z).
        if (pTarget)
        {
            if (auto pTr = pTarget->GetComponent<Engine::Transform>())
            {
                const Engine::Vector3 t = pTr->GetPosition();
                const float dx = t.x - vPos.x;
                const float dz = t.z - vPos.z;
                if (dx * dx + dz * dz > 1e-6f)
                {
                    const float fDesired = atan2f(-dx, -dz);
                    float fDelta = fDesired - transform.GetRY();
                    while (fDelta >  PI) fDelta -= 2.f * PI;   // wrap to [-PI, PI]
                    while (fDelta < -PI) fDelta += 2.f * PI;
                    const float fMaxTurn = kHomingTurnRate * fDeltaTime;
                    if (fDelta >  fMaxTurn) fDelta =  fMaxTurn;
                    if (fDelta < -fMaxTurn) fDelta = -fMaxTurn;
                    transform.SetRY(transform.GetRY() + fDelta);
                }
            }
        }

        // Advance forward along the (possibly steered) heading -- identical to
        // StraightMovement, so with no enemy the missile flies straight.
        transform.AddPosition(transform.GetAxis(Engine::AXIS_TYPE::Y) * (fSpeed * fDeltaTime));
    }
}

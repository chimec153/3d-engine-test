#include "AimedMovement.h"
#include "EnemyTargeting.h"
#include "Bindable/Transform.h"
#include "GameObject/GameObject.h"
#include <cmath>

namespace Client
{
    void AimedMovement::Update(Engine::Transform& transform,
                               float fSpeed, float fDeltaTime)
    {
        // Lock onto the nearest enemy exactly once, on the first frame the
        // bullet moves (its spawn position/yaw are guaranteed set by then).
        if (!m_bAimed)
        {
            m_bAimed = true;

            const Engine::Vector3 vPos = transform.GetPosition();

            // Pick the target once per the weapon's aim mode (shared with
            // HomingMovement via FindTargetEnemy) and snap the heading straight
            // at it. Yaw convention matches Player aim: forward =
            // (-sin yaw, 0, -cos yaw), so the yaw facing a direction is
            // atan2(-dir.x, -dir.z). With no enemy we leave the spawn heading
            // untouched (flies straight as fired).
            if (auto pTarget = FindTargetEnemy(vPos, m_eAimMode))
            {
                if (auto pTr = pTarget->GetComponent<Engine::Transform>())
                {
                    const Engine::Vector3 e = pTr->GetPosition();
                    const float dx = e.x - vPos.x;
                    const float dz = e.z - vPos.z;
                    if (dx * dx + dz * dz > 1e-6f)
                        transform.SetRY(atan2f(-dx, -dz));
                }
            }
        }

        // Advance forward along the locked heading — identical to
        // StraightMovement, so the bullet never changes course after aiming.
        transform.AddPosition(transform.GetAxis(Engine::AXIS_TYPE::Y) * (fSpeed * fDeltaTime));
    }
}

#include "OrbitalMovement.h"
#include "Bindable/Transform.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    OrbitalMovement::OrbitalMovement(std::weak_ptr<Engine::Transform> pOwner)
        : m_pOwner(std::move(pOwner))
    {
    }

    void OrbitalMovement::OnAttached(Engine::Transform& transform)
    {
        // Player::SpawnWeapon rotates each instance around Y to spread phases;
        // capture that yaw as the initial orbit angle.
        m_fAngle = transform.GetRY();
    }

    void OrbitalMovement::Update(Engine::Transform& transform,
                                 float fSpeed, float fDeltaTime)
    {
        // fSpeed is angular rad/sec for orbital (same field as Straight's
        // linear speed; the convention matches WeaponDef::fProjectileSpeed).
        m_fAngle += fDeltaTime * fSpeed;

        auto pOwner = m_pOwner.lock();
        if (!pOwner) return;

        const Engine::Vector3 vCenter = pOwner->GetPosition();
        const float ox = std::cos(m_fAngle) * m_fRadius;
        const float oz = std::sin(m_fAngle) * m_fRadius;
        transform.SetPosition(
            vCenter.x + ox,
            vCenter.y + m_fYOffset,
            vCenter.z + oz);

        // Face along the tangent so spiral/triangle visuals point outward.
        // Tangent yaw at θ is θ + π/2, combined with the engine's
        // forward=(-sin, 0, -cos) convention.
        transform.SetRY(-m_fAngle - 1.5707963f);
    }
}

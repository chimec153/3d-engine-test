#include "OrbitalMovement.h"
#include "../WeaponData.h"   // kMaxOrbitRadius
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

        // A spiralling orbit (radial speed > 0) is a fire-and-spiral
        // projectile, not a shield: pin the centre to the owner's spawn
        // position so it winds outward from there instead of trailing the
        // moving player. SetRadialSpeed runs before OnAttached in
        // Bullet::Configure, so m_fRadialSpeed is already known here.
        if (m_fRadialSpeed > 0.f)
        {
            if (auto pOwner = m_pOwner.lock())
            {
                const Engine::Vector3 c = pOwner->GetPosition();
                m_fCenter[0] = c.x; m_fCenter[1] = c.y; m_fCenter[2] = c.z;
                m_bFixedCenter = true;
            }
        }
    }

    void OrbitalMovement::Update(Engine::Transform& transform,
                                 float fSpeed, float fDeltaTime)
    {
        // fSpeed is angular rad/sec for orbital (same field as Straight's
        // linear speed; the convention matches WeaponDef::fProjectileSpeed).
        m_fAngle += fDeltaTime * fSpeed;
        // Spiral outward when a radial speed was set (0 = fixed-radius orbit).
        m_fRadius += fDeltaTime * m_fRadialSpeed;

        // SpiralOut winds outward from its fixed spawn pivot; a plain orbit
        // tracks the live owner position (a shield that follows the player).
        float cx, cy, cz;
        if (m_bFixedCenter)
        {
            cx = m_fCenter[0]; cy = m_fCenter[1]; cz = m_fCenter[2];
        }
        else
        {
            auto pOwner = m_pOwner.lock();
            if (!pOwner) return;
            const Engine::Vector3 vCenter = pOwner->GetPosition();
            cx = vCenter.x; cy = vCenter.y; cz = vCenter.z;
        }

        const float ox = std::cos(m_fAngle) * m_fRadius;
        const float oz = std::sin(m_fAngle) * m_fRadius;
        transform.SetPosition(cx + ox, cy + m_fYOffset, cz + oz);

        // Face along the tangent so spiral/triangle visuals point outward.
        // Tangent yaw at θ is θ + π/2, combined with the engine's
        // forward=(-sin, 0, -cos) convention.
        transform.SetRY(-m_fAngle - 1.5707963f);
    }

    bool OrbitalMovement::WantsDespawn() const
    {
        // Only a spiralling orbit (radial speed > 0) grows past the cap; a
        // fixed-radius orbit never trips this even with a large base radius.
        return m_fRadialSpeed > 0.f && m_fRadius > kMaxOrbitRadius;
    }
}

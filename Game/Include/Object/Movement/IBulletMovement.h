#pragma once
#include "../WeaponData.h"   // AimMode

namespace Engine
{
    class Transform;
}

namespace Client
{
    // Strategy interface for Bullet movement. One concrete strategy per
    // MovementType — Bullet owns a unique_ptr and delegates per-frame motion.
    // Per-strategy state (orbit angle, spiral phase, etc.) lives on the
    // concrete class so Bullet no longer carries movement-specific fields.
    class IBulletMovement
    {
    public:
        virtual ~IBulletMovement() = default;

        // Per-frame update. fSpeed is the bullet's *current* linear speed
        // (already advanced by acceleration in Bullet::Update). Orbital
        // strategies interpret it as angular rad/sec — same convention as
        // WeaponDef::fProjectileSpeed.
        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) = 0;

        // Hook called once right after the strategy is attached to a freshly
        // configured bullet. Orbital uses it to seed the starting angle from
        // the transform yaw so multiple orbs spread instead of stacking.
        virtual void OnAttached(Engine::Transform& /*transform*/) {}

        // Player::SpawnWeapon pushes a vertical muzzle offset after Configure
        // so the orbit crosses enemy bodies. Forwarded from Bullet via this
        // virtual to avoid a type-narrowed cast — non-orbital strategies
        // ignore it.
        virtual void SetYOffset(float /*f*/) {}

        // Orbit radius in world units (Bullet forwards WeaponDef::fOrbitRadius
        // after Configure). 0 makes the projectile sit on the owner and
        // follow it. Only OrbitalMovement uses this; other strategies ignore.
        virtual void SetRadius(float /*f*/) {}

        // Orbital-only: radial growth in world units/sec (Bullet forwards
        // WeaponDef::fRadialSpeed). >0 spirals the orbit outward; other
        // strategies ignore it.
        virtual void SetRadialSpeed(float /*f*/) {}

        // Target-selection mode (Bullet forwards WeaponDef::eAimMode). Only the
        // auto-targeting movers (Aimed / Homing) use it to pick which enemy to
        // lock; every other strategy ignores it.
        virtual void SetAimMode(AimMode /*e*/) {}

        // True once the strategy decides the projectile should despawn — a
        // spiralling orbit returns true after its radius passes
        // kMaxOrbitRadius. Bullet::Update polls this each frame after Update.
        // Default never despawns (motion stops only at the lifetime cap).
        virtual bool WantsDespawn() const { return false; }
    };
}

#pragma once

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
    };
}

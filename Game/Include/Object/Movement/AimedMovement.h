#pragma once
#include "IBulletMovement.h"

namespace Client
{
    // Fire-and-forget aimed projectile. On its first Update it locks onto the
    // nearest active enemy in the scene (same lookup as HomingMovement), snaps
    // its heading straight at that enemy once, then flies forward forever —
    // unlike HomingMovement it never re-targets. With no enemy present it keeps
    // the heading it spawned with (flies straight as fired).
    //
    // Aiming runs on the first Update, NOT OnAttached: the Cooldown spawn path
    // sets the bullet's position/yaw *after* Bullet::Configure (which calls
    // OnAttached), so OnAttached would read a stale position and have its yaw
    // overwritten. By the first Update the spawn transform is fully set.
    class AimedMovement final : public IBulletMovement
    {
    public:
        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) override;

        // Bullet forwards WeaponDef::eAimMode after Configure; selects which
        // enemy the one-shot lock targets. Defaults to Nearest (legacy).
        virtual void SetAimMode(AimMode e) override { m_eAimMode = e; }

    private:
        bool    m_bAimed   = false;            // heading locked on the first Update
        AimMode m_eAimMode = AimMode::Nearest;
    };
}

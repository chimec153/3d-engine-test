#pragma once
#include "IBulletMovement.h"
#include <memory>

namespace Engine
{
    class Transform;
}

namespace Client
{
    // Circles an owner transform (player) at a fixed radius. Holds all the
    // orbital-only state that used to clutter Bullet: owner pointer, angle
    // accumulator, radius, and the Y offset Player pushes via SetYOffset.
    class OrbitalMovement final : public IBulletMovement
    {
    public:
        explicit OrbitalMovement(std::weak_ptr<Engine::Transform> pOwner);

        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) override;

        // Seed starting angle from the bullet transform's yaw so multiple
        // orbs fan out instead of stacking on phase 0.
        virtual void OnAttached(Engine::Transform& transform) override;

        virtual void SetYOffset(float f) override { m_fYOffset = f; }

        // 0 = orbit collapses to the owner's centre, so the projectile
        // follows the player instead of circling them.
        virtual void SetRadius(float f) override { m_fRadius = f; }

        // >0 grows m_fRadius each frame so the orbit spirals outward.
        virtual void SetRadialSpeed(float f) override { m_fRadialSpeed = f; }

        // Despawn the spiralling orb once its radius passes kMaxOrbitRadius.
        virtual bool WantsDespawn() const override;

    private:
        std::weak_ptr<Engine::Transform> m_pOwner;
        // Outside the player's OBB body so a blocked enemy sits in the
        // orb's collision arc — same value the legacy Bullet used.
        float m_fRadius      = 0.9f;
        float m_fRadialSpeed = 0.f;   // world units/sec the radius grows
        float m_fAngle       = 0.f;
        float m_fYOffset     = 0.f;
        // SpiralOut (radial speed > 0) detaches from the player: OnAttached
        // pins the centre to the owner's spawn position so it winds outward
        // from there instead of trailing the moving player. A fixed-radius
        // orbit (radial speed 0) leaves this unset and keeps following.
        bool  m_bFixedCenter = false;
        float m_fCenter[3]   = { 0.f, 0.f, 0.f };
    };
}

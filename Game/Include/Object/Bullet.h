#pragma once
#include "GameObject\GameObject.h"
#include "WeaponData.h"
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class ColliderSphere;
    class Particle;
    class Collider;
}

namespace Client
{
    // Phase E5 — Bullet migrated from Drawable to GameObject. Configure()
    // is called by Player::SpawnWeapon right after CreateGameObject<Bullet>;
    // it specialises the projectile to the WeaponDef's movement / on-hit /
    // shape and bakes the level-up damage / speed multipliers.
    class Bullet :
        public Engine::GameObject
    {
    public:
        Bullet();
        virtual ~Bullet() override = default;

        // Specialise this bullet to a weapon def at a specific level.
        //   pOwner is the player Transform used by Orbital movement to
        //   anchor the circular path. Other movement types ignore it.
        void Configure(const WeaponDef& def, int iLevel,
                       std::weak_ptr<Engine::Transform> pOwner);

        // Enemy::OnCollision reads this to apply per-weapon damage instead
        // of the legacy hard-coded -1 HP.
        int GetDamage() const { return m_iDamage; }

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;
        std::shared_ptr<Engine::Particle>              m_pTrail;

        // Behaviour copied from WeaponDef at Configure time.
        MovementType m_eMovement = MovementType::Straight;
        OnHitEvent   m_eOnHit    = OnHitEvent::Vanish;
        FireMode     m_eFireMode = FireMode::Cooldown;

        int          m_iDamage   = 1;
        float        m_fSpeed    = 8.f;
        float        m_fLifetime = 2.f;
        float        m_fLifeAcc  = 0.f;

        // Orbital state. m_pOwner is the player transform we circle around;
        // weak so the bullet stays safe if the player is gone (we just
        // freeze in place — the lifetime guard will clean us up).
        std::weak_ptr<Engine::Transform> m_pOwner;
        // Slightly outside the player's OBB body (half-width ~0.25 +
        // some clearance) so an enemy that's been blocked by the body
        // sits exactly in the orb's collision arc. Larger values look
        // nicer visually but stop intersecting the row of enemies
        // pressed against the player.
        float m_fOrbitRadius  = 0.9f;
        float m_fOrbitAngle   = 0.f;   // radians

        // Spiral phase accumulator. Position-offset = right * amp * sin(t * freq).
        float m_fSpiralTime   = 0.f;

        // Multiply tracking: spawned children inherit weapon id/level but
        // are flagged as children so they hit-Vanish (no infinite split).
        int  m_iWeaponId = -1;
        int  m_iLevel    = 1;
        bool m_bIsChild  = false;

        void OnBeginCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void ApplyShape(ProjectileShape eShape, unsigned int uColorRGB);
        void SpawnSplitChildren();
    };
}

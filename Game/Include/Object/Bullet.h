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
    class IBulletMovement;

    // Phase E5 — Bullet migrated from Drawable to GameObject. Configure()
    // is called by Player::SpawnWeapon right after CreateGameObject<Bullet>;
    // it specialises the projectile to the WeaponDef's movement / on-hit /
    // shape and bakes the level-up damage / speed multipliers.
    class Bullet :
        public Engine::GameObject
    {
    public:
        Bullet();
        virtual ~Bullet() override;

        // Specialise this bullet to a weapon def at a specific level.
        //   pOwner is the player Transform used by Orbital movement to
        //   anchor the circular path. Other movement types ignore it.
        void Configure(const WeaponDef& def, int iLevel,
                       std::weak_ptr<Engine::Transform> pOwner);

        // Enemy::OnCollision reads this to apply per-weapon damage instead
        // of the legacy hard-coded -1 HP.
        int GetDamage() const { return m_iDamage; }

        // Multiply split children remember the enemy that spawned them and
        // must never interact with it again — otherwise the two children,
        // born inside that enemy's collider, instantly re-hit it. Both
        // Enemy::OnCollision (skip damage) and Bullet::OnBeginCollision
        // (skip vanish) consult this. The raw pointer is used only for
        // identity comparison, never dereferenced, so a since-freed enemy
        // is harmless (the child lives only a frame or two).
        bool IsIgnoring(const Engine::GameObject* pObj) const
        {
            return m_pIgnoreTarget != nullptr && m_pIgnoreTarget == pObj;
        }

        // Vertical offset applied to the orbital path relative to the
        // owner's pivot. Player calls this with its muzzle-Y offset
        // (-0.7 today) so the orb circles at enemy collider height.
        // Forwarded to the movement strategy — no-op for non-orbital types.
        void SetOrbitYOffset(float f);

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;
        std::shared_ptr<Engine::Particle>              m_pTrail;

        // Per-frame motion delegated to a Strategy. Concrete strategy owns
        // its own state (orbit angle, spiral phase, owner pointer) so this
        // class no longer carries movement-specific fields.
        std::unique_ptr<IBulletMovement> m_pMovement;

        // Behaviour copied from WeaponDef at Configure time.
        OnHitEvent   m_eOnHit    = OnHitEvent::Vanish;
        FireMode     m_eFireMode = FireMode::Cooldown;

        int          m_iDamage       = 1;
        float        m_fSpeed        = 8.f;
        // Speed delta per second. Update applies speed += accel * dt
        // each frame, so positive = accelerate, negative = decelerate.
        // 0 keeps the legacy constant-speed behaviour.
        float        m_fAcceleration = 0.f;
        float        m_fLifetime     = 2.f;
        float        m_fLifeAcc      = 0.f;

        // Owner transform kept here so SpawnSplitChildren can re-thread
        // it into the children's Configure call.
        std::weak_ptr<Engine::Transform> m_pOwner;

        // Multiply tracking: spawned children inherit weapon id/level but
        // are flagged as children so they hit-Vanish (no infinite split).
        int  m_iWeaponId = -1;
        int  m_iLevel    = 1;
        bool m_bIsChild  = false;
        // Enemy a split child must not interact with (the one that spawned
        // it). nullptr for normal bullets. See IsIgnoring.
        Engine::GameObject* m_pIgnoreTarget = nullptr;

        void OnBeginCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void ApplyShape(ProjectileShape eShape, unsigned int uColorRGB);
        void SpawnSplitChildren(Engine::GameObject* pIgnore);
    };
}

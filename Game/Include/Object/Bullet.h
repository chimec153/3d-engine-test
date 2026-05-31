#pragma once
#include "GameObject\GameObject.h"
#include "WeaponData.h"
#include "Vector3.h"
#include <memory>
#include <vector>
#include <deque>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class ColliderSphere;
    class Collider;
    class VoxelWorld;
}

namespace Client
{
    class IBulletMovement;
    class IImpactEffect;

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

        // Scale this bullet's damage by a multiplier (>=0). Player applies its
        // attack-up stat and crit roll here right after Configure, so the
        // boosted value is what Enemy::OnCollision reads. Floors at 1.
        void ScaleDamage(float fMul);

        // Multiply split children remember the enemy that spawned them and
        // must never interact with it again — otherwise the two children,
        // born inside that enemy's collider, instantly re-hit it. Both
        // Enemy::OnCollision (skip damage) and Bullet::OnBeginCollision
        // (skip vanish) consult this. The raw pointer is used only for
        // identity comparison, never dereferenced, so a since-freed enemy
        // is harmless (the child lives only a frame or two).
        bool IsIgnoring(const Engine::GameObject* pObj) const
        {
            if (m_pIgnoreTarget != nullptr && m_pIgnoreTarget == pObj) return true;
            // Chain bullets ignore every enemy already struck this flight, so
            // they neither re-damage nor chain back to it (Enemy::OnCollision
            // and Bullet::OnBeginCollision both consult this).
            for (const Engine::GameObject* p : m_vHitEnemies)
                if (p == pObj) return true;
            return false;
        }

        // True for a damage-over-time zone (OnHitEvent::Field). Enemy reads
        // this to apply periodic tick damage on collision STAY instead of a
        // one-shot hit on BEGIN.
        bool IsField() const { return m_eOnHit == OnHitEvent::Field; }

        // True when this weapon deals damage over time (Enemy ticks it on
        // collision STAY) instead of a one-shot BEGIN hit: any weapon with a
        // tick interval, plus Field zones (which tick even at the default).
        bool TicksDamage() const { return IsField() || m_fTickInterval > 0.f; }

        // Effective DoT tick interval (seconds) Enemy spaces tick damage by.
        // Only consulted when TicksDamage(); a Field zone with no explicit
        // interval falls back to the legacy 0.5s.
        float GetTickInterval() const { return m_fTickInterval > 0.f ? m_fTickInterval : 0.5f; }

        // Vertical offset applied to the orbital path relative to the
        // owner's pivot. Player calls this with its muzzle-Y offset
        // (-0.7 today) so the orb circles at enemy collider height.
        // Forwarded to the movement strategy — no-op for non-orbital types.
        void SetOrbitYOffset(float f);

        // Borrow the scene's voxel world so a Reflect-type bullet can bounce
        // off walls. nullptr (the default) leaves the bullet phasing through
        // voxels exactly as before. Player sets it right after Configure.
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;

        // Tracer trail: short position history (distance-gated) drawn as a
        // camera-facing additive ribbon by TrailRenderManager. [0] = head
        // (newest), back = tail. Replaces the old per-bullet Particle trail
        // (which cost a compute dispatch each). m_vColor matches the weapon.
        std::deque<Engine::Vector3> m_trail;
        Engine::Vector3             m_vColor{ 1.f, 0.85f, 0.2f };
        TrailStyle                  m_eTrailStyle = TrailStyle::Tracer;

        // Per-frame motion delegated to a Strategy. Concrete strategy owns
        // its own state (orbit angle, spiral phase, owner pointer) so this
        // class no longer carries movement-specific fields.
        std::unique_ptr<IBulletMovement> m_pMovement;

        // Weapon impact modules (Knockback / Gather) built from the WeaponDef
        // at Configure time. OnBeginCollision invokes each on every enemy hit.
        // Empty for a plain Damage-only weapon. Damage itself is applied by
        // Enemy::OnCollision, so it produces no entry here.
        std::vector<std::unique_ptr<IImpactEffect>> m_pImpactEffects;

        // Behaviour copied from WeaponDef at Configure time.
        OnHitEvent   m_eOnHit    = OnHitEvent::Vanish;
        FireMode     m_eFireMode = FireMode::Cooldown;

        int          m_iDamage       = 1;
        // Max enemies this bullet may hit before despawning (0 = unlimited);
        // m_iHitCount tallies distinct enemy BEGIN hits. See OnBeginCollision.
        int          m_iMaxHits      = 0;
        int          m_iHitCount     = 0;
        // DoT tick interval (seconds), copied raw from
        // WeaponDef::fDamageInterval at Configure. 0 = no ticking (single
        // BEGIN hit); >0 = tick on STAY every interval. See TicksDamage().
        float        m_fTickInterval = 0.f;
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

        // Multiply tracking: spawned children inherit the weapon id / level
        // (to re-spawn the next generation) and one less split depth.
        int  m_iWeaponId = -1;
        int  m_iLevel    = 1;
        // Multiply: remaining split generations. A bullet with depth > 0
        // splits (and despawns) on hit, handing depth-1 to its children;
        // at depth 0 it just despawns. Bounds the 2^depth cascade.
        int  m_iSplitDepth = 0;
        // Enemy a split child must not interact with (the one that spawned
        // it). nullptr for normal bullets. See IsIgnoring.
        Engine::GameObject* m_pIgnoreTarget = nullptr;

        // Chain on-hit: enemies already struck this flight. Used both to skip
        // re-damage/re-hit (via IsIgnoring) and to exclude them when picking
        // the next chain target. Raw pointers for identity only (never
        // dereferenced). Empty for every non-Chain weapon.
        std::vector<Engine::GameObject*> m_vHitEnemies;

        // Voxel-wall reflection — only the Reflect on-hit bounces off walls.
        // m_pVoxelWorld is borrowed from the scene (nullptr = phase through,
        // the legacy behaviour); m_eMovement gates the bounce to forward-
        // travelling bullets (Orbital is angular, Fixed is stationary); and
        // m_fRadius pads the look-ahead so the bounce reads at the surface
        // instead of after the centre embeds.
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;
        MovementType        m_eMovement   = MovementType::Straight;
        float               m_fRadius     = 0.125f;

        void OnBeginCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void ApplyShape(ProjectileShape eShape, unsigned int uColorRGB);
        void SpawnSplitChildren(Engine::GameObject* pIgnore);
        // Returns true when the bullet bounced off a wall this frame (the
        // heading was reflected and the normal forward step should be
        // skipped). No-op / false for non-Reflect bullets.
        bool TryVoxelReflect(float fDeltaTime);
    };
}

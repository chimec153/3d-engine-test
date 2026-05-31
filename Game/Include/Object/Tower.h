#pragma once
#include "GameObject/GameObject.h"
#include "Vector3.h"
#include <memory>
#include <vector>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class VoxelWorld;
    class Gauge;
    class ColliderSphere;
    class Collider;
}

namespace Client
{
    struct WeaponDef;
    class Attackable;
    class Bullet;
    class Beam;

    // A player-placed turret. Visually a tall (elongated-cube) block that
    // sits on a floor cell; logically it auto-targets the nearest enemy and
    // fires ITS OWN equipped weapon on that weapon's cooldown. Each tower
    // carries its own weapon id (set at placement from TowerManager's default,
    // re-assignable per-tower in the intermission shop), so towers can fire
    // different weapons. Firing mirrors Player::FireCooldownBurst; the aim yaw
    // comes from the nearest enemy rather than the cursor.
    class Tower :
        public Engine::GameObject
    {
    public:
        Tower();
        virtual ~Tower() override;

        // Borrow the scene voxel world so spawned bullets can reflect off
        // walls (forwarded straight into Bullet::SetVoxelWorld). nullptr is
        // fine — bullets just phase through walls then.
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        // Place the tower on the centre of voxel cell (cx,cz). The mesh base
        // sits on the floor top (y = kWallY); bullets spawn at enemy-collider
        // height.
        void SetCell(int cx, int cz);

        // This tower's equipped weapon (a WeaponDatabase live id). The shop's
        // per-tower loadout section sets this; Tower::Init seeds it from
        // TowerManager's placement default.
        int  GetWeaponId() const   { return m_iWeaponId; }
        void SetWeaponId(int iId)   { m_iWeaponId = iId; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Material>              m_pMaterial;
        // Tower health — enemies (highest-aggro target) melee this down; the
        // tower self-destructs (InActivate) when HP hits 0.
        std::shared_ptr<Attackable>                    m_pAttackable;
        // World-anchored HP bar drawn just above the cube. Two-quad Gauge in
        // pixel-space; each frame we project the tower's head position to
        // screen and push that rect into the Gauge so the bar tracks the
        // tower as the camera moves.
        std::shared_ptr<Engine::Gauge>                 m_pHpBar;
        // Body collider so enemy projectiles (EnemyBullet) can shoot the tower
        // down. Enemy melee bypasses the collision system (Enemy applies it
        // directly via the flow field), so this only catches projectiles.
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;

        Engine::VoxelWorld* m_pVoxelWorld = nullptr;

        // This tower's weapon (live id). Seeded in Init from TowerManager's
        // placement default; reassigned per-tower by the shop.
        int   m_iWeaponId    = -1;
        float m_fCooldownAcc = 0.f;
        int   m_iLevel       = 1;
        // Targeting radius (world units). Enemies farther than this are
        // ignored so a tower only engages what's near it.
        float m_fRange       = 12.f;

        // Sustained weapons spawn persistent instances (orbiting blades / aura)
        // around the tower instead of firing on a cooldown. Tracked so a weapon
        // swap drops the old ones; m_iSustainedForId is the weapon they were
        // spawned for (-1 = none / not yet spawned).
        std::vector<std::weak_ptr<Bullet>> m_vecSustained;
        int   m_iSustainedForId = -1;

        // Straight + Sustained weapons are laser Beams (anchored damaging line),
        // not orbiting bullets. The tower keeps them persistent and drives them
        // each frame toward the nearest enemy (mirrors Player's Beam path; the
        // tower has no cursor, so m_fBeamYaw retains the last aim when idle).
        std::vector<std::weak_ptr<Beam>>   m_vecBeams;
        float m_fBeamYaw = 0.f;

        // BEGIN callback for the body collider — applies an enemy projectile's
        // damage to m_pAttackable and consumes the bullet (mirrors the
        // enemy_bullet branch of Player::CollisionPlayerBodyStay).
        void OnHitByBullet(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);

        // Nearest active "Enemy" within m_fRange; returns false if none.
        bool FindNearestEnemy(Engine::Vector3& vOut) const;
        // Spawn one cooldown burst aimed at vTarget (mirrors the player path).
        void FireAt(const Engine::Vector3& vTarget, const WeaponDef& def);
        // Drop any live Sustained instances and spawn a fresh ring around the
        // tower (mirrors Player::RespawnSustainedInstances; owner = this tower
        // so OrbitalMovement circles the tower).
        void RespawnSustained(const WeaponDef& def);
        // Spawn persistent laser Beam(s) anchored to the tower (Straight +
        // Sustained). DriveBeams aims them at the nearest enemy each frame.
        void RespawnBeams(const WeaponDef& def);
        void DriveBeams(float fDeltaTime);
    };
}

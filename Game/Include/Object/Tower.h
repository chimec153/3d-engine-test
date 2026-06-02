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

        // This tower's weapon level (1..kMaxWeaponLevel). Drives the same
        // ComputeCooldown/ComputeCount/ComputeDamage scaling the player's
        // weapon slots use. Raised by merging two same-weapon towers in the
        // shop (TowerIntermissionUI). SetLevel clamps to the cap and forces a
        // Sustained/Beam respawn so the new instance count takes effect.
        int  GetLevel() const   { return m_iLevel; }
        void SetLevel(int iLevel);

        // Which towers.csv type this tower is (def id). The shop buys a type and
        // the placement controller calls this right after creation; the tower
        // re-applies that type's stats + intrinsic effect over the defaults Init
        // seeded. -1 = the default attack type (FirstOfKind(Attack)), preserving
        // the pre-type-select behaviour. GetTowerDefId feeds the death path so a
        // re-placed destroyed tower keeps its type.
        int  GetTowerDefId() const { return m_iTowerDefId; }
        void SetTowerDefId(int iId);

        // Immediate removal for a shop SELL (no death squish): tears down the
        // tower's owned scene instances (orbiting sustained bullets + beams),
        // then InActivates. The owned-count decrement + gold refund are handled
        // by the shop, so unlike the HP-0 death path this does NOT touch
        // TowerManager (see Tower::Update's break branch).
        void Despawn();

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

        // Death squish — when HP hits 0 the cube does a quick squash-and-stretch
        // (flattens, widens) before bursting into shards. m_fSquish advances each
        // frame; at kSquishTime the shatter fires and the tower deactivates.
        // The slot release / instance teardown happen the moment it breaks.
        bool  m_bSquishing = false;
        float m_fSquish    = 0.f;
        static constexpr float kSquishTime   = 0.18f;
        static constexpr float kSquishFlatY  = 0.20f;
        static constexpr float kSquishWideXZ = 1.40f;

        // This tower's weapon (live id). Seeded in Init from TowerManager's
        // placement default; reassigned per-tower by the shop.
        int   m_iWeaponId    = -1;
        float m_fCooldownAcc = 0.f;
        int   m_iLevel       = 1;
        // towers.csv type id (-1 = default attack type). Set by the placement
        // controller via SetTowerDefId; preserved through death so a re-placed
        // tower keeps its type. m_iSeedBaseHP / m_fSeedBaseDef record what Init
        // applied, so SetTowerDefId can shift HP/defence by the type delta
        // without disturbing the level-up bonus layered on top.
        int   m_iTowerDefId  = -1;
        int   m_iSeedBaseHP  = 0;
        float m_fSeedBaseDef = 0.f;
        // Targeting radius (world units). Enemies farther than this are
        // ignored so a tower only engages what's near it.
        float m_fRange       = 12.f;

        // Base combat stats from towers.csv (TowerDatabase). They modify the
        // tower's equipped WEAPON the same way the player's stats do: m_fAttack
        // scales bullet damage, m_fAttackSpeed scales the fire rate (shortens
        // cooldown), and a per-shot crit roll multiplies damage by m_fCritMult.
        // Seeded in Init; defaults give the pre-CSV behaviour.
        float m_fAttack      = 1.f;
        float m_fAttackSpeed = 1.f;
        float m_fCritChance  = 0.f;
        float m_fCritMult    = 2.f;

        // Intrinsic tower effect from towers.csv, layered onto every bullet this
        // tower fires (on top of the equipped weapon's own effects). 0u =
        // Impact_None = no extra effect; P0/P1 are that effect's params. Seeded
        // in Init from the TowerDef; applied in FireAt / RespawnSustained.
        unsigned int m_uTowerImpact   = 0u;   // Impact_None
        float        m_fTowerEffectP0 = 0.f;
        float        m_fTowerEffectP1 = 0.f;

        // Per-type base body colour (set by SetTowerDefId from the tower kind).
        // The damage-feedback tint in Update lerps from this toward red as HP
        // drops, so each type keeps its identity colour at full health.
        Engine::Vector3 m_vBaseColor{ 0.2f, 0.45f, 0.95f };

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
        // Layer this tower's intrinsic impact effect (m_uTowerImpact) onto a
        // freshly-configured bullet, so a hit runs weapon + tower effects. No-op
        // when the tower has no intrinsic effect (Impact_None).
        void ApplyTowerImpact(Bullet* pBullet);
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

#pragma once
#include "GameObject\GameObject.h"
#include "EnemyData.h"
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class VoxelWorld;
    class ColliderSphere;
    class Collider;
    class Layer;
    class GameObject;
}

namespace Client
{
    class Attackable;
    class FlowField;
    class EnemyMeshRenderer;

    // Tower-defense-style enemy that chases a target GameObject (typically the
    // Player). Steering reads a shared FlowField (one Dijkstra solution for
    // the whole army), and entering a solid cell triggers a per-enemy
    // break-in-place pause sized by BlockBreakTime. The flow field is owned
    // by EnemySpawner and rebuilt when the player crosses cell boundaries.
    class Enemy :
        public Engine::GameObject
    {
    public:
        Enemy();
        virtual ~Enemy() override = default;

    public:
        // Two visual variants — both share the same chase/path/collide
        // behaviour, only the mesh + material colour differ. Pick before Init.
        enum class MESH_KIND { BOX, CAPSULE };

    public:
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }
        void SetFlowField(FlowField* pField) { m_pFlowField = pField; }
        // 2D world — enemies live on the floor layer; the y of the spawn
        // cell is implicit (Client::kWallY for body positioning).
        void SetSpawnCell(int x, int z);
        void SetTarget(const std::shared_ptr<Engine::GameObject>& pTarget) { m_TargetObj = pTarget; }
        void SetSpeed(float fSpeed) { m_fSpeed = fSpeed; }
        float GetSpeed() const { return m_fSpeed; }
        // Spawn-time HP override (sets both max and current). Damage drops
        // m_iHP; reaching 0 deactivates the enemy GameObject.
        void SetMaxHP(int iHP) { m_iMaxHP = iHP; m_iHP = iHP; }
        int  GetHP() const { return m_iHP; }
        int  GetMaxHP() const { return m_iMaxHP; }
        // True for entries from enemies.json bosses[] (set in ApplyDef). The
        // HUD shows a boss HP bar while one of these is alive.
        bool IsBoss() const { return m_bIsBoss; }
        // Display label (enemies.json strName) — used as the boss bar caption.
        const std::string& GetName() const { return m_strName; }
        // Safe to call before or after Init: pre-Init it just stores the
        // kind for Init to pick up; post-Init it swaps the mesh + material
        // colour immediately.
        void SetMeshKind(MESH_KIND e);

        // External knockback / gather push from a weapon's ImpactEffects.
        // Adds a world-space (XZ) impulse that Update integrates with decay
        // before the chase steering, so a struck enemy slides and then
        // resumes pathing from wherever it lands. Y is dropped (2D world).
        void ApplyImpulse(const Engine::Vector3& v) { m_vImpulse += v; m_vImpulse.y = 0.f; }

        // Pull this enemy toward vCentre (weapon Gather ImpactEffect). fFraction
        // is how far along the current distance the decaying slide should land:
        // 1 = exactly on vCentre, 0.5 = halfway. Inverts Update's decay model
        // (slide distance ~= |impulse| / kImpulseDamping) so the enemy stops at
        // the intended spot instead of a fixed distance — no overshoot past the
        // centre. Clamped to [0,1]. Replaces the impulse rather than adding, so
        // repeated pulls (piercing/orbital bullet, multi-projectile) converge
        // instead of stacking and flinging the enemy away.
        void PullToward(const Engine::Vector3& vCentre, float fFraction);

        // Apply a burning status (weapon Burn ImpactEffect). Update ticks DoT
        // for the duration via TakeDamage. Re-applying refreshes to the longer
        // remaining time and the higher tick damage (no multiplicative stack).
        void ApplyBurn(int iDmgPerTick, float fDuration)
        {
            if (fDuration  > m_fBurnRemaining) m_fBurnRemaining = fDuration;
            if (iDmgPerTick > m_iBurnDamage)   m_iBurnDamage    = iDmgPerTick;
        }

        // Apply a slow status (weapon Slow ImpactEffect). Update scales chase
        // speed by m_fSlowFactor for the duration. Re-applying refreshes to the
        // longer remaining time and the stronger (lower) factor.
        void ApplySlow(float fFactor, float fDuration)
        {
            if (fDuration > m_fSlowRemaining) m_fSlowRemaining = fDuration;
            if (fFactor   < m_fSlowFactor)    m_fSlowFactor    = fFactor;
        }

        // Highest-aggro active target in a layer (an object carrying an
        // AggroTarget component — the player or a tower). Shared by the
        // spawner (flow-field goal) and the enemy (chase + melee target) so
        // both agree on the single global goal the shared FlowField points at.
        static std::shared_ptr<Engine::GameObject> PickAggroTarget(Engine::Layer* pLayer);

        // Apply a CSV-loaded EnemyDef: writes HP, speed, attack stats and
        // the visual variant in one call. GameScene's spawn loop pulls a
        // row from EnemyDatabase and calls this so a designer can rebalance
        // by editing enemies.csv without touching code.
        void ApplyDef(const EnemyDef& def);

    private:
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;
        FlowField*          m_pFlowField  = nullptr;
        MESH_KIND           m_eMeshKind   = MESH_KIND::BOX;
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<EnemyMeshRenderer>             m_pMeshRenderer;
        std::shared_ptr<Engine::Material>              m_pMaterial;

        // Weak ref so the enemy doesn't keep the player alive past scene exit.
        std::weak_ptr<Engine::GameObject> m_TargetObj;

        // 2D occupied cell. Y is fixed at Client::kWallY for transform
        // positioning so we only track xz.
        int m_iCellX = 0, m_iCellZ = 0;

        float m_fSpeed         = 2.0f;   // cells per second; overwritten by ApplyDef
        float m_fBreakAccum    = 0.f;    // seconds spent breaking the current target cell

        // Death dissolve. On HP<=0 the enemy enters a dissolving state instead
        // of vanishing instantly: m_fDissolve advances each frame and is fed to
        // EnemyMeshRenderer's per-instance PaperTime (the EnemyPSInst dissolve).
        // The GameObject deactivates once fully burned (~kDissolveTime).
        bool  m_bDying    = false;
        float m_fDissolve = 0.f;
        static constexpr float kDissolveTime = 3.0f;

        // Death squish — on the killing blow the body does a quick squash-and-
        // stretch (flattens vertically, widens) before bursting into shards.
        // m_fSquish advances each frame; at kSquishTime the shatter fires and
        // the GameObject deactivates. m_fDeathBaseScale caches the pre-squish
        // uniform scale so the curve and the shard size aren't read from the
        // already-deformed transform.
        bool  m_bSquishing      = false;
        float m_fSquish         = 0.f;
        float m_fDeathBaseScale = 1.f;
        static constexpr float kSquishTime   = 0.18f;  // seconds
        static constexpr float kSquishFlatY  = 0.20f;  // final Y scale factor
        static constexpr float kSquishWideXZ = 1.40f;  // final XZ scale factor

        // The enemy's true uniform scale (set by ApplyDef from the hitbox
        // radius). Both squish effects animate around this rather than the
        // live transform scale, so a transient hit squish never feeds into
        // the death squish / shard size or accumulates across hits.
        float m_fBaseScale = 1.f;

        // Hit squish — a brief elastic squash-and-stretch on every non-fatal
        // hit (juice). The body quickly squashes vertically + widens, then
        // springs back to m_fBaseScale over kHitSquishTime via a sin pulse
        // (0→1→0). The death squish takes precedence when the hit is fatal.
        bool  m_bHitSquish      = false;
        float m_fHitSquish      = 0.f;
        static constexpr float kHitSquishTime   = 0.14f;  // seconds
        static constexpr float kHitSquishFlatY  = 0.72f;  // peak Y scale factor
        static constexpr float kHitSquishWideXZ = 1.18f;  // peak XZ scale factor

        // Health pool. Bullet collisions decrement m_iHP; 0 deactivates
        // the GameObject so the scene's prune pass removes it next frame.
        // GameScene's spawn loop calls ApplyDef with an EnemyDatabase row,
        // which rewrites these via SetMaxHP — the defaults here are only
        // hit when something spawns an Enemy without going through ApplyDef.
        int m_iMaxHP = 10;
        int m_iHP    = m_iMaxHP;

        // Per-archetype pickup rewards (enemies.json goldReward / xpReward).
        // ApplyDef writes these from the EnemyDef; TakeDamage hands them
        // to the dropped Orb on death so a brute (4 gold / 3 xp) and a
        // swarmling (1 / 1) feel meaningfully different to harvest.
        int m_iGoldReward = 1;
        int m_iXpReward   = 1;

        // Display name (enemies.json strName), copied in ApplyDef. Used by the
        // HUD's boss HP bar caption.
        std::string m_strName;

        // === Phase 2: behavior + specials ===
        //
        // m_strBehavior chooses the per-frame movement branch in Update:
        // "chase" (default, all old code path), "dash", or "ranged_kite".
        // The presence of non-zero special params (fExplode*, fDash*,
        // fProj*) tells the tick functions to actually do something —
        // a chase enemy with non-zero fExplodeRadius is a bomber.
        std::string m_strBehavior = "chase";

        // Dash (dasher) — Chase → Telegraph (pause + colour flash) →
        //                  Dashing (locked direction, fast move). Cooldown
        //                  before another dash can trigger.
        enum class DashState { Chase, Telegraph, Dashing };
        DashState        m_eDashState        = DashState::Chase;
        float            m_fDashSpeed        = 0.f;   // cells/sec
        float            m_fDashCooldown     = 0.f;
        float            m_fDashRange        = 0.f;   // cells — trigger AND distance
        float            m_fDashTelegraph    = 0.f;
        float            m_fDashCooldownAcc  = 0.f;   // ticks down; 0 = ready
        float            m_fDashStateTimer   = 0.f;   // remaining telegraph / dash time
        Engine::Vector3  m_vDashDir;

        // Ranged kite (spitter etc.) — keeps preferredRange from the
        // player and fires an EnemyBullet every fFireCooldown seconds.
        float            m_fProjSpeed        = 0.f;   // cells/sec
        int              m_iProjDamage       = 0;
        float            m_fFireCooldown     = 0.f;
        float            m_fFireCooldownAcc  = 0.f;
        float            m_fPreferredRange   = 0.f;   // cells

        // Explode (bomber) — modifier on chase. Player enters fTriggerRange
        // → fuse lights → after fFuseTime, AoE damage at fExplodeRadius +
        // self-deactivate. Movement freezes while the fuse is lit.
        float            m_fExplodeRadius    = 0.f;   // cells
        int              m_iExplodeDamage    = 0;
        float            m_fFuseTime         = 0.f;
        float            m_fTriggerRange     = 0.f;   // cells
        bool             m_bFuseLit          = false;
        float            m_fFuseAcc          = 0.f;

        // === Phase 3 specials ===
        // Split (splitter; spawns N children on death).
        std::string      m_strSplitId;
        int              m_iSplitCount       = 0;

        // Front shield (shieldbearer; frontal damage reduction).
        // Facing is computed on-the-fly from the current target's bearing,
        // so a shield carrier that loses its target briefly still uses its
        // last-seen orientation rather than a stale cached vector.
        float            m_fShieldArcRad     = 0.f;   // half-arc, radians
        float            m_fShieldReduction  = 0.f;

        // Summon action (summoner; periodically spawns minions).
        std::string      m_strSummonId;
        int              m_iSummonCount      = 0;
        float            m_fSummonCooldown   = 0.f;
        float            m_fSummonCooldownAcc= 0.f;

        // Blink (phantom; periodic teleport toward player).
        float            m_fBlinkCooldown    = 0.f;
        float            m_fBlinkDistance    = 0.f;   // cells
        float            m_fBlinkCooldownAcc = 0.f;

        // === Boss state ===
        bool                  m_bIsBoss          = false;
        std::vector<BossPhase> m_vecPhases;
        int                   m_iCurrentPhase    = -1;   // -1 = unactivated; transitions in Update
        float                 m_fPhaseSpeedMult  = 1.f;
        // Ability cadence — primary ability + (optional) alsoSummon channel.
        enum class AbilityState { Idle, Telegraph, Active };
        AbilityState          m_eAbilityState    = AbilityState::Idle;
        float                 m_fAbilityCdAcc    = 0.f;   // until next telegraph
        float                 m_fAbilityStateAcc = 0.f;   // telegraph remaining / charge duration
        float                 m_fAltSummonCdAcc  = 0.f;
        Engine::Vector3       m_vAbilityDir;

        // Body collider so the player's bullets (tag "bullet_body") can
        // hit us. Set up + callback wired in Init.
        std::shared_ptr<Engine::ColliderSphere> m_pCollider;

        // Melee attack — when the target is within m_fAttackRange the
        // cooldown ticks down and triggers Player::OnHitBy on expiry.
        std::shared_ptr<Attackable> m_pAttackable;
        float m_fAttackRange    = 1.5f;
        float m_fAttackCooldown = 1.0f;
        float m_fAttackAcc      = 0.f;

        // Damage-over-time from a Field weapon. While overlapping a Field
        // bullet, OnFieldStay accumulates time and applies the bullet's damage
        // once per the bullet's own tick interval (WeaponDef::fDamageInterval,
        // read via Bullet::GetTickInterval).
        float m_fFieldDamageAcc = 0.f;

        // Decaying knockback / gather impulse (world XZ). ApplyImpulse adds to
        // it; Update integrates and decays it each frame. Zeroed once it falls
        // below kImpulseStopSq so it doesn't jitter forever.
        Engine::Vector3 m_vImpulse;
        static constexpr float kImpulseDamping = 6.0f;    // per-second decay rate
        static constexpr float kImpulseStopSq  = 0.0025f; // rest threshold (vel²)

        // Burning status (weapon Burn ImpactEffect). m_fBurnRemaining counts
        // down each frame; every kBurnTickInterval the enemy takes m_iBurnDamage
        // via TakeDamage. Same DoT shape as the Field tick above, but self-timed
        // (persists after the bullet is gone) instead of overlap-driven.
        float m_fBurnRemaining = 0.f;
        float m_fBurnTickAcc   = 0.f;
        int   m_iBurnDamage    = 0;
        static constexpr float kBurnTickInterval = 0.5f;
        // Flame VFX cadence — much faster than the damage tick so the burst
        // pool reads as a continuous flame column as the enemy moves.
        float m_fBurnVfxAcc    = 0.f;
        static constexpr float kBurnVfxInterval = 0.1f;

        // Slow status (weapon Slow ImpactEffect). m_fSlowFactor (<=1) scales
        // chase speed while m_fSlowRemaining > 0; factor resets to 1 on expiry.
        float m_fSlowRemaining = 0.f;
        float m_fSlowFactor    = 1.f;

        bool ResolveTargetCell(int& tx, int& tz) const;
        Engine::Vector3 CellCenter(int x, int z) const;

        // Phase 2 behavior ticks. Each returns true if it fully handled
        // this frame's movement and the chase fallback should be skipped.
        // TickExplode is a modifier — returns true once the fuse is lit
        // (movement freezes) so the chase block doesn't fight the fuse.
        bool TickDash       (float fDeltaTime);
        bool TickRangedKite (float fDeltaTime);
        bool TickExplode    (float fDeltaTime);
        // Helper: spawn an EnemyBullet aimed at the current target.
        void FireProjectileAtTarget();
        // Detonate now: AoE damage every enemy / player in range, then
        // self-deactivate. Called from TickExplode when the fuse runs out.
        void Detonate();

        // === Phase 3 ticks / helpers ===
        // Blink modifier on chase (phantom). Returns true if it teleported
        // this frame (the chase block then runs from the new position).
        bool TickBlink     (float fDeltaTime);
        // Summon action on top of ranged-kite movement (summoner).
        void TickSummonAction(float fDeltaTime);
        // Boss phase + ability driver. Returns true when the ability
        // owns this frame (so the chase fallback is skipped).
        bool TickBossPhase (float fDeltaTime);
        // Spawn iCount enemies of strId in a small ring around me.
        // Shared by split-on-death, summoner, boss summon, alsoSummon.
        void SpawnMinions  (const std::string& strId, int iCount, float fRingRadius);
        // Slam: damage every enemy/player inside fRadius. No mesh, no
        // particles — just damage + a small shared VFX burst at centre.
        void ApplySlamDamage(float fRadius, int iDamage);
        // Burst the body into fragment shards (the death "pop"). Uses the cached
        // pre-squish scale so shard size matches the original body, not the
        // flattened one. Called at the end of the death squish.
        void ShatterBody();

    public:
        // Materialise called by spawner to flip on boss visuals
        // (oversized capsule already handled in ApplyDef via hitbox; this
        // hooks up the phase list copy etc.). Currently a tag setter; left
        // public so the spawner can call it after CreateGameObject.
        void MarkAsBoss(const std::vector<BossPhase>& phases)
        {
            m_bIsBoss    = true;
            m_vecPhases  = phases;
            m_iCurrentPhase = -1;
        }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    public:
        // Shared hit pipeline: combat text + hit flash + HP drop + death / orb
        // drop. pSource (optional) is the world-space position the damage came
        // FROM (shieldbearer front-arc check); nullptr = self-applied DoT.
        // Public so external damagers (e.g. the laser Beam) can deal damage
        // directly, not just the Enemy's own collision callbacks.
        void TakeDamage(int iDmg, const Engine::Vector3* pSource = nullptr);
        // Collider BEGIN callback — handles bullet hits (one-shot).
        void OnCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        // Collider STAY callback — periodic tick damage from Field zones.
        void OnFieldStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };
}

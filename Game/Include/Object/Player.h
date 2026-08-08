#pragma once

#include "GameObject\GameObject.h"
#include "State.h"
#include "WeaponData.h"
#include "Weapon.h"
#include <vector>
#include <string>
#include <memory>

namespace Engine
{
    class Drawable;
    class Terrain;
    class ColliderOBB;
    class JointSocket;
    class Particle;
    class ColliderLine;
    class Camera;
    class Transform;
    class MeshRendererComponent;
    class Animation;
    class Mesh;
    class Bindable;
    class Collider;
    class VoxelWorld;
}

namespace Client
{
    class Inventory;
    class Attackable;
    class Trail;
    class Bullet;
    class Pet;
    class Beam;
    class IPlayerLowerState;
    class IPlayerUpperState;

    // Phase E5 — Player migrated from Drawable to GameObject. Components
    // (Transform, MeshRenderer, Animation, Attackable, ColliderOBB,
    // ColliderLine on the camera, etc.) are added in Init via AddComponent.
    // Sword/armor/shadow effects are still Drawable instances — they're
    // accepted as shared_ptr<Drawable> at the existing API boundaries
    // (ChangeWeaponMesh / ChangeArmorMesh) and JointSocket continues to
    // hold the sword Drawable. RollEffect's shadow trails are also still
    // Drawable.
    class Player :
        public Engine::GameObject
    {
    public:
        enum class MOVE_DIR
        {
            LEFT,
            RIGHT,
            UP,
            DOWN,
            END
        };

        // Level-up stat upgrade cards are now data-driven: the option list +
        // display + weight + magnitude live in levelups.csv (LevelUpDatabase),
        // and each card's "key" string selects the effect applied here in
        // ApplyStatUpgrade. (Replaces the old hard-coded StatUpgrade enum.)
    public:
        Player(int iMaxHP, int iAttackMin, int iAttackMax);
        virtual ~Player() override;

        // A heal tower pulse restored iAmount HP to the player. Fires the
        // player's heal feedback: a green screen vignette (RenderManager),
        // a burst of rising green motes around the body, and a green "+N"
        // number above the head. No-op if iAmount<=0.
        void OnHealed(int iAmount);

    private:
        // Phase E5 — Shadow effect entries. After RollEffect's GameObject
        // migration, each entry holds the GameObject and its MeshRenderer
        // (cached for the per-frame fade material update).
        typedef struct _tagShadowInfo
        {
            std::shared_ptr<Engine::GameObject>            pGameObject;
            std::shared_ptr<Engine::MeshRendererComponent> pMeshRenderer;
            int iFrame;

            _tagShadowInfo(std::shared_ptr<Engine::GameObject> p, std::shared_ptr<Engine::MeshRendererComponent> pMR) :
                pGameObject(p)
                , pMeshRenderer(pMR)
                , iFrame(0)
            {
            }
        }SHADOWINFO, *PSHADOWINFO;
        // Phase E5 — Attackable hp/attack stats deferred to Init.
        int m_iInitHP;
        int m_iInitAttackMin;
        int m_iInitAttackMax;
        std::shared_ptr<Attackable> m_pAttackable;

        // Heal feedback — rising green "motes" burst around the body when a
        // heal pulse lands (OnHealed). World-space particles, so they only
        // need positioning at emit time, not per-frame syncing. Armed once in
        // Init (StopEmit + AddEmitCount(1)); OnHealed bursts it.
        std::shared_ptr<Engine::Particle> m_pHealMoteParticle;

        // Phase E5 — Components on this GameObject.
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Animation>             m_pAnimation;

        float m_fSpeed;
        float m_fRollSpeed;
        Engine::Vector3 m_vRollDir;
        std::shared_ptr<Engine::Camera> m_pCamera;
        std::shared_ptr<Engine::Terrain> m_pTerrain;
        // Two independent state machines — lower body (locomotion + roll +
        // hit + death) and upper body (idle / attack). State classes live in
        // PlayerState.h. ChangeLowerState/UpperState below handle anim sync
        // + the original SetState transition gates (Roll/Hit are absorbing
        // unless RollEnd/HitEnd/Die is requested).
        StateMachine<Player, IPlayerLowerState> m_lowerStateMachine{ *this };
        StateMachine<Player, IPlayerUpperState> m_upperStateMachine{ *this };
        MOVE_DIR m_eDir;
        std::list<SHADOWINFO>    m_ShadowList;
        int m_iMaxShadowFrame;
        float m_fCameraDist;
        // Phase E5 — sword / armor / joint sockets removed. They were
        // populated only by ChangeWeaponMesh / ChangeArmorMesh, which were
        // called from Inventory's UpdateEquipSlot — Inventory's body is
        // entirely dead at runtime (CreateDrawable<Inventory> in GameScene
        // is commented out), so these fields stayed null forever and the
        // associated notify callbacks (also commented out) never fired.
        std::shared_ptr<Engine::ColliderOBB> m_pBody;
        std::shared_ptr<Trail> m_pTrail;
        std::shared_ptr<Engine::ColliderLine> m_pCameraLine;
        std::shared_ptr<Inventory> m_pInventory;
        std::shared_ptr<Engine::SoundBindable> m_pFootLSound;
        std::shared_ptr<Engine::SoundBindable> m_pFootRSound;

        // Phase V6 — Player borrows the scene-owned VoxelWorld via raw
        // pointer (no ownership). Set by GameScene::Init right after the
        // Player is constructed.
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;

        // Owned weapons are now heap Weapon objects (Weapon.h), held by
        // shared_ptr. The player holds the ones it owns (equipped + idle
        // inventory, distinguished by Weapon::bEquipped); a weapon assigned to a
        // tower is MOVED out of this vector into the tower (one object, one
        // holder — so no copy-counting / tower-held flag is needed).
        static constexpr int kMaxEquipSlots     = 4;
        static constexpr int kMaxInventorySlots = 6;
        static constexpr int kMaxWeaponSlots    = kMaxEquipSlots + kMaxInventorySlots;
        std::vector<WeaponPtr> m_vecWeaponSlots;

        // Spawn/tear-down each owned weapon's persistent instances to match its
        // location: equipped weapons fire (instances live), inventory weapons
        // don't. Called each frame from Update.
        void ReconcileInstances();

        // Aim mode toggle. Default = false (bullets follow the player's
        // facing direction). Pressing LCTRL flips it; while true the
        // weapon-spawn path traces the mouse cursor onto the player's
        // y-plane and aims along that vector.
        bool m_bMouseAim = false;

        // Footstep marks — distance walked since the last print, and which
        // foot is next (alternates L/R). Drives FootstepManager::Spawn.
        float m_fStrideAccum = 0.f;
        bool  m_bLeftFoot    = false;

        // Damage-feedback state. m_fImpactCooldown throttles the dramatic
        // single-hit reaction (flash + shake + hit-stop) so chained projectile
        // hits don't strobe; m_fHeartAcc paces the low-HP heartbeat SFX.
        float m_fImpactCooldown = 0.f;
        float m_fHeartAcc       = 0.f;

        // Experience / level progression. Orbs dropped by dead enemies
        // feed AddExp on pickup; AddExp pushes overflow back into the
        // next level so a single pickup can never silently drop XP.
        int  m_iLevel       = 1;
        int  m_iXp          = 0;      // XP accumulated in the current level
        int  m_iXpToNext    = 5;      // XP needed to advance to the next level
        // Counter — AddExp increments once per level boundary crossed in
        // a single call (a fat XP pickup can cross multiple levels).
        // LevelUpChoices consumes one per card pick, looping until the
        // counter drains; this avoids the old boolean's lost-level-up
        // bug where AddExp(20) at level 1 would queue only one card.
        int  m_iPendingLevelUps = 0;

        // Stat-upgrade accumulators (the rest live on m_pAttackable: max HP +
        // damage reduction, and on m_fSpeed: move speed). Applied to spawned
        // bullets in FireCooldownBurst.
        float m_fDamageMult = 1.f;   // attack-up multiplier on bullet damage
        int   m_iFlatDamage = 0;     // flat bullet-damage bonus (AttackFlat upgrade)
        float m_fCritChance = 0.f;   // 0..1 chance a bullet crits
        float m_fCritMult   = 2.f;   // crit damage multiplier
        float m_fGoldMult   = 1.f;   // gold-gain multiplier on orb pickup
        float m_fXpMult     = 1.f;   // XP-gain multiplier on orb pickup

    private:
        // Get the aim yaw the next projectile should fly along. Honours
        // m_bMouseAim (raycast onto the player's y-plane) and falls back
        // to the player's facing yaw.
        float ComputeAimYaw() const;
        // Base spawn heading for a weapon per its AimMode (the LCTRL mouse-aim
        // toggle is a global Cursor override). Radial is distributed per
        // projectile by the caller, so it returns the player facing here.
        float ComputeWeaponAimYaw(const WeaponDef& def,
                                  const Engine::Vector3& vPlayerPos) const;
        // Mouse-cursor aim yaw (camera ray onto the player plane). Returns
        // false / leaves outYaw untouched when unavailable. Shared by
        // ComputeAimYaw and ComputeWeaponAimYaw.
        bool TryComputeMouseAimYaw(float& outYaw) const;
        // Spawn one Cooldown shot from a weapon. Reads the weapon level for
        // damage / speed scaling; the weapon's cooldown accumulator is the
        // caller's job.
        void  FireCooldownBurst(const WeaponPtr& slot);
        // Spawn Sustained instances for a weapon (Orbital orbs). Called when
        // the weapon is gained or its level changes — old instances are
        // released first so the count change takes effect.
        void  RespawnSustainedInstances(const WeaponPtr& slot);
        // Spawn / refresh a Follow weapon's pets (movement == Follow). Mirrors
        // RespawnSustainedInstances: drops the old companions and spawns
        // ComputeCount fresh ones ringed around the player.
        void  RespawnPets(const WeaponPtr& slot);
        // Spawn / refresh a Straight+Sustained weapon's laser beam(s). The
        // Player drives them each frame (DriveBeams) with the live cursor aim.
        void  RespawnBeams(const WeaponPtr& slot);
        // Per-frame: anchor each beam at the muzzle and aim it down the
        // weapon's current heading (called from Update).
        void  DriveBeams(float fDeltaTime);
        // Spawn a weapon's live instances (orbs / pets / beams) per its def.
        // Shared by the add-new, add-copy, and level-up paths.
        void  SpawnSlotInstances(const WeaponPtr& slot);
        // Bump a weapon's level in place, applying the one-time evolution at the
        // threshold and re-spawning its live instances. Shared by
        // AddOrLevelUpWeapon (existing slot) and MergeWeapon.
        void  LevelUpSlot(const WeaponPtr& slot);
        // Player damage feedback. TriggerImpactFeedback fires the dramatic
        // single-hit reaction (red flash + camera shake + hit-stop), throttled
        // by m_fImpactCooldown. UpdateDamageFeedback runs each frame: ticks the
        // cooldown, pushes the low-HP overlay strength to the renderer, and
        // paces the low-HP heartbeat SFX.
        void  TriggerImpactFeedback();
        void  UpdateDamageFeedback(float fDeltaTime);

    public:
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        // Weapon-slot API consumed by LevelUpChoices.
        //   AddOrLevelUpWeapon — adds a new slot, or levels an existing
        //                        slot if the player already owns the
        //                        weapon. Sustained instances re-spawn on
        //                        level changes so count/speed bumps take.
        //   GetOwnedWeaponIds   — ids the player already has (LevelUpChoices
        //                        uses this to compose the card pool).
        //   GetOwnedWeaponLevel — current level for a given owned id.
        //   GetWeaponSlotCount  — used to decide whether new-weapon cards
        //                        should appear in the pool (capped at 6).
        void AddOrLevelUpWeapon(int iWeaponId);
        // Add a fresh copy of a weapon as a NEW slot, even if the player
        // already owns it (capped at kMaxWeaponSlots). The shop buy path uses
        // this so duplicate copies can later be combined via MergeWeapon.
        void AddWeaponCopy(int iWeaponId);
        // Combine two owned copies of the same weapon into one: erases the
        // lowest-level copy and levels up the highest-level one (freeing a
        // slot). No-op (returns false) unless at least two copies are owned.
        bool MergeWeapon(int iWeaponId);
        // How many slots currently hold this weapon (0..kMaxWeaponSlots).
        int  CountOwnedWeapon(int iWeaponId) const;
        // Drop an owned weapon (shop sell). Tears down its live instances
        // (sustained orbs / pets / beams) and erases the slot. No-op if the
        // weapon isn't owned.
        void RemoveWeapon(int iWeaponId);
        std::vector<int> GetOwnedWeaponIds() const;
        int  GetOwnedWeaponLevel(int iWeaponId) const;

        // --- Move a weapon between the player and a tower (object ownership) ---
        // DetachWeapon removes ONE owned copy of iWeaponId (preferring an idle
        // inventory copy) and returns it for a tower to hold — the player no
        // longer owns it. Returns nullptr if no copy is owned. AttachWeapon takes
        // a weapon back from a tower (sell / merge-consume / unassign) and parks
        // it in the inventory. Together these give move semantics: a weapon is
        // owned by EITHER the player OR a tower, never both — no copy-counting.
        WeaponPtr DetachWeapon(int iWeaponId);
        void      AttachWeapon(const WeaponPtr& pWeapon);
        bool      HasFreeWeaponSlot() const
        {
            return static_cast<int>(m_vecWeaponSlots.size()) < kMaxWeaponSlots;
        }

        // --- Equip / inventory (shop) -----------------------------------------
        // The player FIRES only equipped, non-tower-held weapons (up to
        // kMaxEquipSlots). Everything else owned sits idle in the inventory.
        static constexpr int GetMaxEquipSlots() { return kMaxEquipSlots; }
        // Weapons the player currently fires (equipped & not on a tower).
        std::vector<int> GetEquippedWeaponIds() const;
        // Owned weapons sitting in the inventory (not equipped, not on a tower).
        std::vector<int> GetInventoryWeaponIds() const;
        // Per-copy LEVELS in the SAME order as the id lists above. Each owned copy
        // has its own level (Weapon::iLevel), and duplicate ids can be at
        // different levels, so the shop reads the level positionally (not by id)
        // to show each icon's true level.
        std::vector<int> GetEquippedWeaponLevels() const;
        std::vector<int> GetInventoryWeaponLevels() const;
        // How many weapons occupy a player equip slot right now (incl. any that
        // are also tower-held — those still count against the cap until moved).
        int  GetEquippedCount() const;
        bool IsWeaponEquipped(int iWeaponId) const;
        // Move an owned weapon between the inventory and a player equip slot.
        // EquipWeapon fails (returns false) when all equip slots are full.
        bool EquipWeapon(int iWeaponId);
        void UnequipWeapon(int iWeaponId);
        int  GetWeaponSlotCount() const { return static_cast<int>(m_vecWeaponSlots.size()); }
        static constexpr int GetMaxWeaponSlots() { return kMaxWeaponSlots; }

        // Experience hooks for Orb pickups. AddExp folds overflow into
        // m_iXpToNext = 5 * level (so each level takes a few more orbs)
        // and raises m_bPendingLevelUp on every boundary crossing.
        void AddExp(int iAmount);
        int  GetExp()      const { return m_iXp; }
        int  GetLevel()    const { return m_iLevel; }
        int  GetXpToNext() const { return m_iXpToNext; }

        // Level-up handshake between Player and the LevelUpChoices UI:
        //   HasPendingLevelUp — true while any card must still be picked.
        //   PendingLevelUpCount — how many picks remain (lets the UI
        //                        re-roll for sequential level-ups
        //                        without closing the modal).
        //   ConsumeLevelUp     — UI calls this with the chosen weapon
        //                        id to apply the boost and decrement
        //                        the pending counter.
        bool HasPendingLevelUp()    const { return m_iPendingLevelUps > 0; }
        int  PendingLevelUpCount()  const { return m_iPendingLevelUps; }
        void ConsumeLevelUp(int iChoice);
        // Apply a chosen stat upgrade and consume one pending level-up.
        // LevelUpChoices calls this when the player picks a card; strKey is the
        // levelups.csv "key" column and fAmount its "amount" (magnitude).
        void ApplyStatUpgrade(const std::string& strKey, float fAmount);

        // Apply a hit from an external attacker. Mirrors the frogclaw
        // collision branch in CollisionPlayerBodyStay so non-collision damage
        // sources (e.g. Enemy periodic melee from Enemy::Update) can route
        // through the same Attackable + Hit/Die state path.
        void OnHitBy(Attackable* pAttacker);

        // HP accessors for HUD readback. Both forward to the Attackable
        // component that actually tracks the values.
        int GetHP()    const;
        int GetMaxHP() const;
        // Restore current HP to full (called when a round is cleared).
        void FullHeal();

        // Stat readback for the intermission shop's stat panel. These mirror
        // the five StatUpgrade choices (move speed / max HP / attack / crit /
        // defense) plus the player's bullet-attack scaling.
        float GetMoveSpeed()  const { return m_fSpeed; }
        float GetDamageMult() const { return m_fDamageMult; }
        float GetCritChance() const { return m_fCritChance; }
        float GetDamageReduction() const;
        // True once a lethal hit has dropped HP to 0 (Attackable::Attack
        // returns true and the Die state latches). Polled by GameOverUI.
        bool IsDead()  const { return GetHP() <= 0; }

    public:
        // Replace the active lower/upper state. Mirrors the original SetState
        // gating: Roll/Hit accept only RollEnd/HitEnd/Die transitions, Attack
        // accepts only AttackEnd, Die is absorbing. Returns true when the
        // change was applied. State classes live in PlayerState.h.
        bool ChangeLowerState(std::unique_ptr<IPlayerLowerState> pNext);
        bool ChangeUpperState(std::unique_ptr<IPlayerUpperState> pNext);
        void UpdateState(float fDeltaTime);

        // Accessors for state classes (PlayerState.cpp implementations).
        const std::shared_ptr<Engine::Animation>& GetAnimation()    const { return m_pAnimation; }
        const std::shared_ptr<Engine::Transform>& GetTransformRef() const { return m_pTransform; }
        const std::shared_ptr<Engine::Terrain>&   GetTerrain()      const { return m_pTerrain; }
        const std::shared_ptr<Engine::ColliderOBB>& GetBody()       const { return m_pBody; }
        const std::shared_ptr<Inventory>&         GetInventory()    const { return m_pInventory; }
        Engine::Vector3 GetRollDir() const { return m_vRollDir; }
        float           GetRollSpeed() const { return m_fRollSpeed; }
        void RollEffect(int iFrame, float fTime, Engine::Bindable* pDrawable);
        void ChangeSequence(const std::string& strSeq);
        void SetRate(float fRate);
        void SetAdditiveSequence(const std::string& strSeq);
        // Phase E5 — ChangeWeaponMesh / ChangeArmorMesh / GetWeapon
        // removed (sword/armor equip path was Inventory-driven and dead).
        void SetInventory(std::shared_ptr<Inventory> pInventory);

        // Convenience accessor — Player exposes its Transform for legacy
        // code paths that still reach in (Inventory's UI camera placement,
        // shadow effect spawning in RollEffect, etc.).
        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        // Target-mode (mouse-aim) state. LCTRL toggles it in Input(); the
        // bottom-left HUD button reads IsMouseAim() to label itself and calls
        // ToggleMouseAim() to flip it exactly like the key.
        bool IsMouseAim() const { return m_bMouseAim; }
        void ToggleMouseAim() { m_bMouseAim = !m_bMouseAim; }

    public:
        void CollisionTerrainStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void CollisionPlayerBodyStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void CollisionCameraLine(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        void CollisionCameraLineLast(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);

    public:
        virtual void Start() override;
        virtual bool Init() override;
        virtual void Input(float fDeltaTime) override;
        virtual void Update(float fDeltaTime) override;
        virtual void FixedUpdate(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
    };

}

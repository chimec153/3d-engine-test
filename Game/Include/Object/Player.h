#pragma once

#include "GameObject\GameObject.h"
#include "State.h"
#include "WeaponData.h"
#include <vector>

namespace Engine
{
    class Drawable;
    class Terrain;
    class ColliderOBB;
    class JointSocket;
    class Particle;
    class ColliderLine;
    class Camera;
    class SoundBindable;
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
    public:
        Player(int iMaxHP, int iAttackMin, int iAttackMax);
        virtual ~Player() override;

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

        // Weapon slots. Each slot tracks one CSV-loaded weapon, its current
        // level, the cooldown accumulator (for FireMode::Cooldown), and the
        // live Sustained instances (Orbital orbs etc. that persist across
        // frames). Capacity is locked to 6 (Vampire-Survivors-style).
        struct WeaponSlot
        {
            int   iWeaponId    = -1;
            int   iLevel       = 1;
            float fCooldownAcc = 0.f;
            std::vector<std::weak_ptr<Bullet>> vecSustainedInstances;
        };
        static constexpr int kMaxWeaponSlots = 6;
        std::vector<WeaponSlot> m_vecWeaponSlots;

        // Aim mode toggle. Default = false (bullets follow the player's
        // facing direction). Pressing LCTRL flips it; while true the
        // weapon-spawn path traces the mouse cursor onto the player's
        // y-plane and aims along that vector.
        bool m_bMouseAim = false;

        // Experience / level progression. Orbs dropped by dead enemies
        // feed AddExp on pickup; AddExp pushes overflow back into the
        // next level so a single pickup can never silently drop XP.
        int  m_iLevel       = 1;
        int  m_iXp          = 0;      // XP accumulated in the current level
        int  m_iXpToNext    = 5;      // XP needed to advance to the next level
        // Latched flag — AddExp sets this when a level boundary was just
        // crossed. LevelUpChoices consumes the flag (and applies a stat
        // boost) once the player picks a card.
        bool m_bPendingLevelUp = false;

    private:
        // Get the aim yaw the next projectile should fly along. Honours
        // m_bMouseAim (raycast onto the player's y-plane) and falls back
        // to the player's facing yaw.
        float ComputeAimYaw() const;
        // Spawn one Cooldown shot from a slot. Reads slot.iLevel for damage
        // / speed scaling; the slot's cooldown accumulator is the caller's
        // job.
        void  FireCooldownBurst(const WeaponSlot& slot);
        // Spawn Sustained instances for a slot (Orbital orbs). Called when
        // the weapon is gained or its level changes — old instances are
        // released first so the count change takes effect.
        void  RespawnSustainedInstances(WeaponSlot& slot);

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
        std::vector<int> GetOwnedWeaponIds() const;
        int  GetOwnedWeaponLevel(int iWeaponId) const;
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
        //   HasPendingLevelUp — true while a card must still be picked.
        //   ConsumeLevelUp     — UI calls this with the choice index
        //                        (0/1/2) to apply the boost and clear
        //                        the flag.
        bool HasPendingLevelUp() const { return m_bPendingLevelUp; }
        void ConsumeLevelUp(int iChoice);

        // Apply a hit from an external attacker. Mirrors the frogclaw
        // collision branch in CollisionPlayerBodyStay so non-collision damage
        // sources (e.g. Enemy periodic melee from Enemy::Update) can route
        // through the same Attackable + Hit/Die state path.
        void OnHitBy(Attackable* pAttacker);

        // HP accessors for HUD readback. Both forward to the Attackable
        // component that actually tracks the values.
        int GetHP()    const;
        int GetMaxHP() const;

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

#pragma once

#include "GameObject\GameObject.h"
#include "State.h"

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

        // Periodic forward-fire — spawns a Bullet GameObject (with a
        // Particle trail attached) every m_fFireInterval seconds.
        float m_fFireInterval = 0.3f;
        float m_fFireAcc      = 0.f;

        // Experience pool. Orbs dropped by dead enemies feed AddExp on pickup.
        int m_iExp = 0;

    private:
        void SpawnBullet();

    public:
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        // Experience hooks for Orb pickups.
        void AddExp(int iAmount) { m_iExp += iAmount; }
        int  GetExp() const      { return m_iExp; }

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

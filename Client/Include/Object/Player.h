#pragma once

#include "GameObject\GameObject.h"

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
}

namespace Client
{
    class Inventory;
    class Attackable;
    class Trail;

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
        enum class PLAYER_STATE
        {
            IDLE,
            RUN,
            ROLL,
            ROLL_END,
            HIT,
            HIT_END,
            DIE,
            END
        };

        enum class PLAYER_UPPER_BODY_STATE
        {
            IDLE,
            ATTACK,
            ATTACK_END,
            GUN_IDLE,
            END
        };

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
        float m_fAccel;
        float m_fFallSpeed;
        float m_fRollSpeed;
        Engine::Vector3 m_vRollDir;
        std::shared_ptr<Engine::Camera> m_pCamera;
        std::shared_ptr<Engine::Terrain> m_pTerrain;
        PLAYER_STATE m_eState;
        PLAYER_UPPER_BODY_STATE m_eUpperState;
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
        bool m_bCanJump;
        std::shared_ptr<Engine::ColliderLine> m_pCameraLine;
        std::shared_ptr<Inventory> m_pInventory;
        std::shared_ptr<Engine::SoundBindable> m_pFootLSound;
        std::shared_ptr<Engine::SoundBindable> m_pFootRSound;

    public:
        bool SetState(PLAYER_STATE eState);
        bool SetUpperBodyState(PLAYER_UPPER_BODY_STATE eState);
        void UpdateState(float fDeltaTime);
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

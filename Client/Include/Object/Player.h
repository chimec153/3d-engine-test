#pragma once

#include "Attackable.h"

namespace Engine
{
    class Terrain;
    class ColliderOBB;
    class JointSocket;
    class Particle;
    class ColliderLine;
    class Camera;
    class SoundBindable;
}

namespace Client
{
    class Inventory;

    class Player :
        public Attackable
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
        typedef struct _tagShadowInfo
        {
            std::shared_ptr<Engine::Drawable> pDrawable;
            int iFrame;

            _tagShadowInfo(std::shared_ptr<Engine::Drawable> pDrawable) :
                pDrawable(pDrawable)
                , iFrame(0)
            {
            }
        }SHADOWINFO, *PSHADOWINFO;
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
        std::shared_ptr<Engine::Drawable> m_pSword;
        std::shared_ptr<Engine::ColliderOBB> m_pBody;
        std::shared_ptr<Engine::ColliderOBB> m_pSwordBody;
        std::shared_ptr<class Trail> m_pTrail;
        bool m_bCanJump;
        std::shared_ptr<Engine::JointSocket> m_pJointSocket;
        std::shared_ptr<Engine::Particle> m_pSwordParticle;
        std::shared_ptr<Engine::ColliderLine> m_pCameraLine;
        std::shared_ptr<Inventory> m_pInventory;
        std::shared_ptr<Engine::JointSocket> m_pJointSocketArmor;
        std::shared_ptr<Engine::SoundBindable> m_pSwordSound;
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
        void ChangeWeaponMesh(std::shared_ptr<Engine::Drawable> pDrawable);
        void ChangeArmorMesh(std::shared_ptr<Engine::Drawable> pDrawable);
        std::shared_ptr<Engine::Drawable> GetWeapon()   const;
        void SetInventory(std::shared_ptr<Inventory> pInventory);

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
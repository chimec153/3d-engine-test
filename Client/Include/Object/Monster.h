#pragma once
#include "GameObject\GameObject.h"
namespace Engine
{
    class ColliderSphere;
    class ColliderOBB;
    class SoundBindable;
    class Agent;
    class Transform;
    class MeshRendererComponent;
    class Animation;
}
namespace Client
{
    class Attackable;

    // Phase E5 — Monster migrated from Drawable to GameObject. Components
    // (Transform, MeshRenderer, Animation, Attackable, ColliderSphere)
    // are added in Init via AddComponent; mesh loading flows through
    // Drawable::LoadIntoMeshRenderer for the bridged loader. Currently
    // dead at runtime (the GameScene CreateDrawable<Monster> call is
    // commented out), so this migration validates the pattern that Player
    // will follow.
    class Monster :
        public Engine::GameObject
    {
        enum class MONSTER_STATE
        {
            IDLE,
            RUN,
            ATTACK,
            HIT,
            HIT_END,
            DIE,
            END
        };
    public:
        Monster(int iMaxHP, int iAttackMin, int iAttackMax);
        virtual ~Monster();

    private:
        // Phase E5 — ctor params stored for use in Init (Component creation
        // requires GameObject::Init to have run first).
        int m_iInitHP;
        int m_iInitAttackMin;
        int m_iInitAttackMax;

        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Animation>             m_pAnimation;
        std::shared_ptr<Attackable>                    m_pAttackable;

        std::shared_ptr<Engine::Agent> m_pAgent;
        std::shared_ptr<Engine::ColliderSphere> m_pBody;
        std::shared_ptr<Engine::ColliderOBB> m_pClawBody;
        MONSTER_STATE m_eState;
        std::shared_ptr<Engine::SoundBindable> m_pAttackSound;

    public:
        bool SetState(MONSTER_STATE eState);

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    public:
        void CollisionEnter(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };

}

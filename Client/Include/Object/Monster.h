#pragma once
#include "Attackable.h"
namespace Engine
{
    class ColliderSphere;
    class ColliderOBB;
    class SoundBindable;
}
namespace Client
{
    class Monster :
        public Attackable
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
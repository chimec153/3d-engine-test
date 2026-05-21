#pragma once
#include "GameObject\GameObject.h"
#include "State.h"
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
    template <typename T> class IState;
    using IMonsterState = IState<class Monster>;

    // Phase E5 — Monster migrated from Drawable to GameObject. AI state lives
    // in m_pState (Strategy/State pattern, see MonsterState.h) instead of the
    // old MONSTER_STATE enum + per-frame switch. Each state owns its own
    // transition rules, which fixed the "monster moves during attack" bug
    // (Update used to force-switch states by distance every frame).
    class Monster :
        public Engine::GameObject
    {
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
        std::shared_ptr<Engine::SoundBindable> m_pAttackSound;

        // AI state machine. Generic over Monster — same StateMachine<TOwner>
        // template that Player uses for its lower/upper body machines.
        // ChangeState below adds the animation sync layer the template
        // itself stays out of.
        StateMachine<Monster> m_stateMachine{ *this };

        // Set by the Attack-anim "AttackEnd" notify and read by
        // MonsterAttackState::Update to decide when to leave Attack.
        bool m_bAttackAnimFinished = false;

    public:
        // Replace the current AI state. Safe to call from anywhere (Update,
        // collision callbacks, notifies). Null pNext is a no-op.
        void ChangeState(std::unique_ptr<IMonsterState> pNext);

        // State-pattern hooks need at component-level data. Exposing as
        // accessors keeps MonsterState.cpp from needing to friend-include
        // every Monster internal.
        const std::shared_ptr<Engine::Transform>& GetTransform()  const { return m_pTransform; }
        const std::shared_ptr<Engine::Animation>& GetAnimation()  const { return m_pAnimation; }
        const std::shared_ptr<Engine::Agent>&     GetAgent()      const { return m_pAgent; }
        const std::shared_ptr<Engine::ColliderOBB>& GetClawBody() const { return m_pClawBody; }
        const std::shared_ptr<Attackable>&        GetAttackable() const { return m_pAttackable; }

        bool ConsumeAttackFinished()
        {
            const bool b = m_bAttackAnimFinished;
            m_bAttackAnimFinished = false;
            return b;
        }
        void ResetAttackFinished() { m_bAttackAnimFinished = false; }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    public:
        void CollisionEnter(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };

}

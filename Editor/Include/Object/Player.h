#pragma once
#include "GameObject\GameObject.h"

namespace Engine
{
    class Agent;
    class Animation;
    class Transform;
    class MeshRendererComponent;
}

namespace Editor
{
    // Phase E7 — Player migrated from Drawable to GameObject. Editor uses
    // this as a navmesh-following test agent: ImguiManager spawns one
    // (CreateGameObject<Player>) on a navmesh click, attaches an Agent
    // Component pointing at the click target, and the Agent's Update
    // walks the GameObject along the path.
    class Player : public Engine::GameObject
    {
    public:
        Player();
        virtual ~Player() override = default;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Animation>             m_pAnimation;
        std::shared_ptr<Engine::Agent>                 m_pAgent;

    public:
        virtual bool Init() override;

    public:
        // Editor-side accessors mirroring Drawable's old API surface that
        // ImguiManager calls into.
        void SetAgent(const std::shared_ptr<Engine::Agent>& pAgent);
        const std::shared_ptr<Engine::Agent>& GetAgent() const { return m_pAgent; }
        void Move(const Engine::Vector3& vPos);

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }
    };
}

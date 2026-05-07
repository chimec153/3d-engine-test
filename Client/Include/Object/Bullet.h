#pragma once
#include "GameObject\GameObject.h"

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class ColliderSphere;
}

namespace Client
{
    // Phase E5 — Bullet migrated from Drawable to GameObject. The GameObject
    // hosts the Transform / MeshRenderer / ColliderSphere components; the
    // Bullet override provides the speed-driven forward motion.
    class Bullet :
        public Engine::GameObject
    {
    public:
        Bullet();
        virtual ~Bullet() override = default;

    private:
        float m_fSpeed;
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;

    public:
        // Convenience accessor — Player wires the bullet's spawn position
        // and direction via the transform right after creation.
        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
    };
}

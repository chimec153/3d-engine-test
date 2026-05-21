#pragma once
#include "GameObject\GameObject.h"

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class ColliderSphere;
    class Particle;
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
        // GPU particle emitter that trails behind the bullet. Has its own
        // Transform (Particle owns its emitter anchor) so Bullet::Update
        // mirrors the bullet's position/rotation each frame.
        std::shared_ptr<Engine::Particle>              m_pTrail;

    public:
        // Convenience accessor — Player wires the bullet's spawn position
        // and direction via the transform right after creation.
        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
    };
}

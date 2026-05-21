#pragma once
#include "GameObject/GameObject.h"
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class ColliderSphere;
    class Collider;
}

namespace Client
{
    // Pickup orb dropped by dead enemies. Sits at the drop position until a
    // PlayerBody collider touches it, at which point it grants experience to
    // the player GameObject and deactivates itself.
    class Orb : public Engine::GameObject
    {
    public:
        Orb();
        virtual ~Orb() override = default;

        void SetExpValue(int iExp) { m_iExp = iExp; }

        virtual bool Init() override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;
        int m_iExp = 1;

        void OnCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };
}

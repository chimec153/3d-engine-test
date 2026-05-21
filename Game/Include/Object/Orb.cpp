#include "Orb.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Orb, Orb)
#include "Player.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/Sphere.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Types.h"

namespace Client
{
    namespace
    {
        // Shared low-poly sphere geometry — every orb in the scene reuses it.
        std::shared_ptr<Engine::Mesh> EnsureOrbMesh()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Mesh>("OrbMesh"))
                return pCached;

            std::vector<Engine::VertexStandard> verts;
            std::vector<unsigned int>           inds;
            Engine::Sphere::CreateSphereVertex<Engine::VertexStandard>(6, 12, verts);
            Engine::Sphere::GetSphereVertexTexcoord<Engine::VertexStandard>(6, 12, verts);
            Engine::Sphere::CreateSphereIndex(6, 12, inds);

            return Engine::StaticCreateBindable<Engine::Mesh>("OrbMesh", verts, inds);
        }

        // One shared material so all orbs sit in the same instancing bucket
        // and the representative material colour matches every member —
        // same rationale as EnsureEnemyMaterial in Enemy.cpp.
        std::shared_ptr<Engine::Material> EnsureOrbMaterial()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Material>("OrbMaterial"))
                return pCached;

            auto pMat = Engine::StaticCreateBindable<Engine::Material>("OrbMaterial");
            if (!pMat) return Engine::StaticFindBindable<Engine::Material>("OrbMaterial");

            pMat->SetDiffuseColor (1.0f, 0.85f, 0.2f, 1.f);
            pMat->SetEmissiveColor({ 1.0f, 0.7f, 0.1f, 1.f });
            return pMat;
        }
    }

    Orb::Orb() = default;

    bool Orb::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Sphere helper emits a radius-0.5 sphere centred at the origin; shrink
        // it via the transform so the orb reads as a small floating bead.
        if (m_pTransform) m_pTransform->SetScale(0.3f, 0.3f, 0.3f);

        auto pMat = EnsureOrbMaterial();

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(EnsureOrbMesh());
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            if (pMat)
            {
                m_pMeshRenderer->SetMaterial(pMat);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, pMat);
            }
        }

        m_pCollider = AddComponent<Engine::ColliderSphere>("orb_body");
        if (m_pCollider)
        {
            m_pCollider->SetRadius(0.25f);
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Orb::OnCollision);
        }

        return true;
    }

    void Orb::OnCollision(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float /*fDeltaTime*/)
    {
        if (!pDest || pDest->GetTag() != "PlayerBody") return;

        if (auto* pOwner = pDest->GetGameObjectOwner())
        {
            if (auto* pPlayer = dynamic_cast<Player*>(pOwner))
                pPlayer->AddExp(m_iExp);
        }

        InActivate();
    }
}

#include "Player.h"
#include "Bindable/Mesh.h"
#include "Bindable/Transform.h"
#include "Bindable/Animation.h"
#include "Bindable/Agent.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Animation/Skeleton.h"
#include "Component/MeshRendererComponent.h"

namespace Editor
{
    Player::Player() = default;

    bool Player::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
        m_pAnimation    = AddComponent<Engine::Animation>("anim");

        std::shared_ptr<Engine::Mesh> pMesh =
            Engine::StaticFindBindable<Engine::Mesh>("Medieval");

        std::shared_ptr<Engine::Skeleton> pSkeleton = std::make_shared<Engine::Skeleton>();
        pSkeleton->LoadFromPath("Walking.skel", MESH_PATH);
        if (m_pAnimation) m_pAnimation->SetSkeleton(pSkeleton);

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(pMesh);
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>("AlphaNoUVPS"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            m_pMeshRenderer->SetAnimation(m_pAnimation);
        }
        return true;
    }

    void Player::SetAgent(const std::shared_ptr<Engine::Agent>& pAgent)
    {
        m_pAgent = pAgent;
        if (m_pAgent)
        {
            // Agent was already constructed by NavMesh::CreateAgent with our
            // Transform; attach as a Component so its Update fires each frame
            // and drives the GameObject along the navmesh path.
            AddComponent(std::static_pointer_cast<Engine::Component>(m_pAgent));
        }
    }

    void Player::Move(const Engine::Vector3& vPos)
    {
        if (m_pAgent) m_pAgent->SetTargetPos(vPos);
    }
}

#include "Player.h"
#include "Bindable/NavMesh.h"
#include "Bindable/Agent.h"
#include "Core/Window.h"
#include "Bindable/TransformBuffer.h"
#include "Bindable/Animation.h"
#include "Animation/Sequence.h"
#include "Animation/Skeleton.h"
#include "Bindable/Mesh.h"
#include "Animation/JointSocket.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/ColliderLine.h"
#include "Bindable/Sphere.h"
#include "Scene/SceneManager.h"
#include "Bindable/BlendState.h"
#include "Bindable/PaperBurn.h"

Player::Player() :
	Drawable()
	, m_pAgent(nullptr)
	, m_pAnimation(CreateBindable<Engine::Animation>("anim"))
#ifdef _DEBUG
	, m_pSphere()
#endif
{
	std::shared_ptr<Engine::Mesh> pMesh = CreateBindable<Engine::Mesh>("mesh", "Medieval.mesh", MESH_PATH);

	//std::shared_ptr<Engine::Sequence> pSequence = std::make_shared<Engine::Sequence>();

	//if (pSequence)
	//{
	//	pSequence->LoadFromPath("MedievalCharacterArmature_Idle_Neutral.seq", MESH_PATH);
	//}

	//std::shared_ptr<Engine::Sequence> pWalkSequence = std::make_shared<Engine::Sequence>();

	//if (pWalkSequence)
	//{
	//	pWalkSequence->LoadFromPath("MedievalCharacterArmature_Walk.seq", MESH_PATH);

	//	pWalkSequence->UseRootMotion();
	//}

	//std::shared_ptr<Engine::Skeleton> pSkeleton = std::make_shared<Engine::Skeleton>();
	//
	//if (pSkeleton)
	//{
	//	pSkeleton->LoadFromPath("Medieval.skel", MESH_PATH);
	//	m_pAnimation->SetSkeleton(pSkeleton);
	//}

	//m_pAnimation->AddSequance("idle", pSequence);
	//m_pAnimation->AddSequance("walk", pWalkSequence);

	FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
	FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
	FindAndAddBind<Engine::InputLayout>("Standard");
	FindAndAddBind<Engine::Topology>("TriangleList");

	GetTransform()->SetRX(Engine::DegToRad(180.f));
}

Player::Player(const Player& player) :
	Drawable(player)
	, m_pAgent(std::static_pointer_cast<Engine::Agent>(FindChild(Engine::BINDABLE_TYPE::AGENT)))
	, m_pAnimation(std::static_pointer_cast<Engine::Animation>(FindChild(Engine::BINDABLE_TYPE::ANIMATION)))
	, m_pFootLineCollider()
#ifdef _DEBUG
	, m_pSphere()
#endif
{
	if (m_pAgent)
	{
		m_pAgent->SetTransform(GetTransform());
	}
}

void Player::CreateAgent(std::shared_ptr<Engine::NavMesh> pNavMesh, const Engine::Vector3& pos)
{
	m_pAgent = CreateBindable<Engine::Agent>("agent", GetTransform(), pNavMesh.get(), pos);
}

void Player::Move(const Engine::Vector3& pos)
{
	if (m_pAgent)
	{
		m_pAgent->SetTargetPos(pos);
	}
}

void Player::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (m_pAgent && m_pAnimation)
	{
		const Engine::Vector3& vVelocity = m_pAgent->GetAgentVelocity();

		if (vVelocity.Length() < epsilon)
		{
			m_pAnimation->ChangeSequence("idle");
		}
		else
		{
			m_pAnimation->ChangeSequence("walk");
		}
	}
}

void Player::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
}

std::shared_ptr<Engine::Bindable> Player::Clone()
{
	return std::make_shared<Player>(*this);
}

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
	std::shared_ptr<Engine::Mesh> pMesh = CreateBindable<Engine::Mesh>("mesh", "BodyVer4.mesh", MESH_PATH);

	std::shared_ptr<Engine::Sequence> pSequence = std::make_shared<Engine::Sequence>();

	pSequence->LoadFromPath("BodyVer4_idle.seq", MESH_PATH);

	std::shared_ptr<Engine::Sequence> pWalkSequence = std::make_shared<Engine::Sequence>();

	pWalkSequence->LoadFromPath("co00_161_WalkLoopTake 001.seq", MESH_PATH);

	std::shared_ptr<Engine::Skeleton> pSkeleton = std::make_shared<Engine::Skeleton>();

	pSkeleton->LoadFromPath("BodyVer4.skel", MESH_PATH);

	m_pAnimation->SetSkeleton(pSkeleton);

	pWalkSequence->UseRootMotion();

	m_pAnimation->AddSequance("idle", pSequence);
	m_pAnimation->AddSequance("walk", pWalkSequence);

	FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
	FindAndAddBind<Engine::PixelShader>("PaperBurnPS");
	FindAndAddBind<Engine::InputLayout>("Standard");
	FindAndAddBind<Engine::Topology>("TriangleList");

	GetTransform()->SetScale(Engine::Vector3(0.02f, 0.02f, 0.02f));
	GetTransform()->SetRX(Engine::DegToRad(180.f));

	std::shared_ptr<Engine::PaperBurn> pPaperBurn = CreateBindable<Engine::PaperBurn>("PaperBurn", Engine::StaticFindBindable<Engine::Texture>("PaperBurn"));

	pPaperBurn->SetStartColor(Engine::Red);
	pPaperBurn->SetMidColor(Engine::Yellow);
	pPaperBurn->SetFinalColor(Engine::White);
	pPaperBurn->SetStartRate(0.4f);
	pPaperBurn->SetMidRate(0.55f);
	pPaperBurn->SetFinalRate(0.6f);
	pPaperBurn->SetEndRate(0.65f);
	pPaperBurn->SetMaxTime(8.f);

	//std::shared_ptr<Drawable> pWeapon = CreateBindable<Drawable>("weapon");

	//pWeapon->CreateBindable<Engine::Mesh>("weapon_mesh", "Bow005.mesh");
#ifdef _DEBUG
	/*std::shared_ptr<Engine::Transform> pSphereTransform = m_pSphere->GetTransform();

	if (pSphereTransform)
	{
		pSphereTransform->SetScale(0.75f, 0.75f, 0.75f);
	}*/
#endif
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

	Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();
	
	std::shared_ptr<Engine::JointSocket> pJointSocket = std::make_shared<Engine::JointSocket>();

	pJointSocket->SetPosition(Engine::Vector3(0.f, 0.f, 0.f));
	pJointSocket->SetRotation(Engine::Vector3(-0.7, 0.f, 0.f));

	m_pAnimation->AddSocket(117, pJointSocket);

	std::shared_ptr<Drawable> pHead = pScene->CreateDrawable<Drawable>("head", pScene->FindLayer(ALPHA_LAYER));

	std::shared_ptr<Engine::Mesh> pHeadMesh = pHead->CreateBindable<Engine::Mesh>("head_mesh", "FaceIs255boneAndHair.mesh", MESH_PATH);

	std::shared_ptr<Engine::Animation> pHeadAni = pHead->CreateBindable<Engine::Animation>("head_ani");

	std::shared_ptr<Engine::Sequence> pHeadSequence = std::make_shared<Engine::Sequence>();

	pHeadSequence->LoadFromPath("FaceIs255boneAndHair_idle.seq", MESH_PATH);

	pHeadAni->AddSequance("idle", pHeadSequence);

	std::shared_ptr<Engine::Skeleton> pHeadSkeleton = std::make_shared<Engine::Skeleton>();

	pHeadSkeleton->LoadFromPath("FaceIs255boneAndHair.skel", MESH_PATH);

	pHeadAni->SetSkeleton(pHeadSkeleton);

	pHead->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
	pHead->FindAndAddBind<Engine::PixelShader>("PaperBurnPS");
	pHead->FindAndAddBind<Engine::InputLayout>("Standard");
	pHead->FindAndAddBind<Engine::Topology>("TriangleList");
	//pHead->FindAndAddBind<Engine::BlendState>("AlphaBlend");

	pJointSocket->SetDrawable(pHead);

#ifdef _DEBUG
	for (int i = 0; i < 2; ++i)
	{
		m_pSphere[i] = pScene->CreateDrawable<Engine::Sphere>("DebugSphere", pScene->FindLayer(DEFAULT_LAYER), 16, 16);

		m_pSphere[i]->GetMaterial()->SetDiffuseColor(i, 0.f, 1.f, 1.f);
	}
#endif

	for (int i = 0; i < 2; ++i)
	{
		char strSphere[TEXT_LEN] = {};

		sprintf_s(strSphere, "Foot_%d", i + 1);

		std::shared_ptr<Engine::Drawable> pDrawable = pScene->CreateDrawable<Engine::Sphere>(strSphere, pScene->FindLayer(DEFAULT_LAYER), 32, 32);

		std::shared_ptr<Engine::Transform> pTransform = pDrawable->GetTransform();

		/*m_pFootLineCollider[i] = pDrawable->CreateBindable<Engine::ColliderLine>("LeftFootLineCollider");

		m_pFootLineCollider[i]->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &Player::CollisionStay);

		m_pFootLineCollider[i]->SetStartOffset(0.f, 100.f, 0.f);

		m_pFootLineCollider[i]->SetEndOffset(0.f, -100.f, 0.f);*/

		std::shared_ptr<Engine::JointSocket> pFootSocket = std::make_shared<Engine::JointSocket>();

		pFootSocket->SetDrawable(pDrawable);

		pFootSocket->SetScale(5.f, 5.f, 5.f);

		m_pAnimation->AddSocket(i, pFootSocket);
	}

	//m_pAnimation->AddIkInfo(6, 1);
	//m_pAnimation->AddIkInfo(13, 1);

	std::shared_ptr<Engine::PaperBurn> pPaperBurn = std::static_pointer_cast<Engine::PaperBurn>(FindChild(Engine::BINDABLE_TYPE::PAPERBURN));

	pPaperBurn->StartPaperBurn();
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

	if (m_pAgent)
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

	std::shared_ptr<Engine::Transform> pTransform = GetTransform();

	if (pTransform)
	{
		const Engine::Vector3& vPos = pTransform->GetPosition();

		const Engine::Vector3& vScale = pTransform->GetScale();

		const Engine::Vector3& vRotation = pTransform->GetRotation();

		const Engine::Matrix& matInverse = Engine::Matrix::TranslateFromVector(-vPos) * Engine::Matrix::RotationXYZ(vRotation).Transpose() * Engine::Matrix::Scaling(1.f / vScale);

		for (int i = 0; i < 2; ++i)
		{
			m_pAnimation->SetIkPosition(6 + 7 * i, matInverse.TransformCoord(m_pSphere[i]->GetTransform()->GetPosition()));
		}
	}
}

void Player::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{/*
	Engine::Drawable* pParent = static_cast<Engine::Drawable*>(pSrc->GetParent());

	if (pParent)
	{
		std::shared_ptr<Engine::Transform> pColliderTransform = pParent->GetTransform();

		if (pColliderTransform)
		{
			const Engine::Vector3 vScale = pColliderTransform->GetScale();

			Engine::Matrix matInverse = Engine::Matrix::TranslateFromVector(-pColliderTransform->GetPosition()) * Engine::Matrix::RotationXYZ(pColliderTransform->GetRotation()).Transpose() * Engine::Matrix::Scaling({ 1.f / vScale.x, 1.f / vScale.y, 1.f / vScale.z });

			m_pAnimation->SetIkPosition(matInverse.TransformCoord(pSrc->GetCross()));
		}
	}

#ifdef _DEBUG
	for (int i = 0; i < 2; ++i)
	{
		if (m_pFootLineCollider[i].get() == pSrc ||
			m_pFootLineCollider[i].get() == pDest)
		{
			std::shared_ptr<Engine::Transform> pTransform = m_pSphere[i]->GetTransform();

			if (pTransform)
			{
				pTransform->SetPosition(pSrc->GetCross());
			}
			break;
		}
	}
#endif*/
}

std::shared_ptr<Engine::Bindable> Player::Clone()
{
	return std::make_shared<Player>(*this);
}

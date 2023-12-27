#include "Monster.h"
#include "Bindable/TransformBuffer.h"
#include "Bindable/NavMesh.h"
#include "Scene/Scene.h"
#include "Bindable/Agent.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Animation.h"
#include "Bindable/Topology.h"
#include "Bindable/InputLayout.h"
#include "Bindable/ColliderOBB.h"
#include "Animation/JointSocket.h"
#include "Animation/Sequence.h"
#include "Bindable/Mesh.h"
#include "Attackable.h"
#include "Bindable/Particle.h"

Client::Monster::Monster(int iMaxHP, int iAttackMin, int iAttackMax)	:
	Attackable(iMaxHP, iAttackMin, iAttackMax)
{
}

Client::Monster::~Monster()
{
}

bool Client::Monster::SetState(MONSTER_STATE eState)
{
	switch (m_eState)
	{
	case Client::Monster::MONSTER_STATE::IDLE:
		break;
	case Client::Monster::MONSTER_STATE::RUN:
		switch (eState)
		{
		case Client::Monster::MONSTER_STATE::IDLE:
			break;
		case Client::Monster::MONSTER_STATE::RUN:
			return false;
		case Client::Monster::MONSTER_STATE::ATTACK:
			break;
		case Client::Monster::MONSTER_STATE::END:
			break;
		default:
			break;
		}
		break;
	case Client::Monster::MONSTER_STATE::ATTACK:
		break;
	case Client::Monster::MONSTER_STATE::HIT:
		switch (eState)
		{
		case Client::Monster::MONSTER_STATE::IDLE:
			break;
		case Client::Monster::MONSTER_STATE::RUN:
			break;
		case Client::Monster::MONSTER_STATE::ATTACK:
			break;
		case Client::Monster::MONSTER_STATE::HIT:
			break;
		case Client::Monster::MONSTER_STATE::HIT_END:
			break;
		case Client::Monster::MONSTER_STATE::END:
			break;
		default:
			break;
		}
		break;
	case Client::Monster::MONSTER_STATE::DIE:
		return false;
	case Client::Monster::MONSTER_STATE::END:
		break;
	default:
		break;
	}

	m_eState = eState;

	switch (m_eState)
	{
	case Client::Monster::MONSTER_STATE::IDLE:
		GetAnimation()->ChangeSequence("FrogArmature|Frog_Idle");
		break;
	case Client::Monster::MONSTER_STATE::RUN:
		GetAnimation()->ChangeSequence("FrogArmature|Frog_Jump");
		break;
	case Client::Monster::MONSTER_STATE::ATTACK:
		GetAnimation()->ChangeSequence("FrogArmature|Frog_Attack");
		break;
	case Client::Monster::MONSTER_STATE::HIT:
		GetAnimation()->ChangeSequence("FrogArmature|Frog_Jump");
		break;
	case Client::Monster::MONSTER_STATE::DIE:
		GetAnimation()->ChangeSequence("FrogArmature|Frog_Death");
		if (m_pClawBody)
		{
			m_pClawBody->InActivate();
		}
		return false;
	case Client::Monster::MONSTER_STATE::END:
		break;
	default:
		break;
	}

	return false;
}

bool Client::Monster::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	std::shared_ptr<Engine::Mesh> pFrogMesh = FindAndAddBind<Engine::Mesh>("Frog");

	pFrogMesh->UsePaperBurn();

	std::shared_ptr<Engine::Animation> pFrogAnimation = CreateBindable<Engine::Animation>("FrogAnimation");

	pFrogAnimation->FindAndAddSequence("FrogArmature|Frog_Idle");
	pFrogAnimation->FindAndAddSequence("FrogArmature|Frog_Jump");
	pFrogAnimation->FindAndAddSequence("FrogArmature|Frog_Attack");

	pFrogAnimation->SetLoop("FrogArmature|Frog_Attack");
	pFrogAnimation->SetLoop("FrogArmature|Frog_Jump");
	
	pFrogAnimation->FindAndAddSequence("FrogArmature|Frog_Death");

	pFrogAnimation->SetSkeleton("Frog");

	FindAndAddBind<Engine::VertexShader>(STANDARD_ANIM_VS);
	FindAndAddBind<Engine::PixelShader>(STANDARD_SOLID_PS);
	FindAndAddBind<Engine::Topology>("TriangleList");
	FindAndAddBind<Engine::InputLayout>("Standard");

	GetAnimation()->GetCurrentSequence()->Loop();

	GetTransform()->SetScale(0.25f, 0.25f, 0.25f);
	GetTransform()->SetPosition(5.f, 30.f, 5.f);

	std::shared_ptr<Engine::Bindable> pTerrain = std::static_pointer_cast<Engine::NavMesh>(GetScene()->FindBindable("Terrain"));

	if (pTerrain)
	{
		std::shared_ptr<Engine::NavMesh> pNavMesh = std::static_pointer_cast<Engine::NavMesh>(pTerrain->FindChild(Engine::BINDABLE_TYPE::NAV_MESH));

		if (pNavMesh)
		{
			m_pAgent = pNavMesh->CreateAgent(GetTag() + "agent", GetTransform(), GetTransform()->GetPosition());
		}
	}

	m_pBody = CreateBindable<Engine::ColliderSphere>(GetTag() + "body");

	m_pBody->SetRadius(0.5f);

	m_pBody->SetOffset({ 0.f, 0.25f, 0.f });

	m_pBody->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Monster::CollisionEnter);

	std::shared_ptr<Attackable> pClaw = GetScene()->CreateDrawable<Attackable>("Claw", GetScene()->FindLayer(DEFAULT_LAYER), 30, 5, 10);

	m_pClawBody = pClaw->CreateBindable<Engine::ColliderOBB>(GetTag() + "clawbody");

	m_pClawBody->SetScaleOffset({0.2f, 0.1f, 0.2f});

	std::shared_ptr<Engine::JointSocket> pSocket = GetAnimation()->AddSocket(5, pClaw);

	pSocket->SetPosition({ 0.f, 0.f, 1.f });

	//std::shared_ptr<Engine::Notify> pDieNotify = pFrogAnimation->AddNotify("FrogArmature|Frog_Death", "PaperBurn", 0.8f);

	//pDieNotify->SetCallBack(
	//	[this](int, float, Engine::Bindable*) 
	//	{
	//		StartPaperBurn();
	//		GetParticle()->StopEmit();
	//	}
	//);

	return true;
}

void Client::Monster::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	std::shared_ptr<Engine::Drawable> pPlayer = std::static_pointer_cast<Engine::Drawable>(GetScene()->FindBindable("player"));

	if (pPlayer)
	{
		std::shared_ptr<Engine::Transform> pPlayerTransform = pPlayer->GetTransform();

		const Engine::Vector3& vPlayerPos = pPlayerTransform->GetPosition();

		const Engine::Vector3& vPos = GetTransform()->GetPosition();

		float fTargetDist = (vPos - vPlayerPos).Length();

		if (fTargetDist < 1.f)
		{
			SetState(MONSTER_STATE::ATTACK);

			m_pAgent->SetTargetPos(vPos);
		}

		else if (fTargetDist < 10.f)
		{
			SetState(MONSTER_STATE::RUN);

			m_pAgent->SetTargetPos(vPlayerPos);
		}

		else
		{
			SetState(MONSTER_STATE::IDLE);

			m_pAgent->SetTargetPos(vPos);
		}
	}
	else
	{
		SetState(MONSTER_STATE::IDLE);
	}

	switch (m_eState)
	{
	case Client::Monster::MONSTER_STATE::IDLE:
		break;
	case Client::Monster::MONSTER_STATE::RUN:
		break;
	case Client::Monster::MONSTER_STATE::ATTACK:
		break;
	case Client::Monster::MONSTER_STATE::HIT: 
	{
	}
		break;
	case Client::Monster::MONSTER_STATE::HIT_END:
		break;
	case Client::Monster::MONSTER_STATE::END:
		break;
	default:
		break;
	}
}
void Client::Monster::CollisionEnter(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
	if (pDest->GetTag() == "sword_body")
	{
		Attackable* pWeapon = static_cast<Attackable*>(pDest->GetParent());

		if (pWeapon->Attack(this))
		{
			SetState(MONSTER_STATE::DIE);

			m_pAgent->InActivate();
		}
		else
		{
			SetState(MONSTER_STATE::HIT);
		}
	}
}

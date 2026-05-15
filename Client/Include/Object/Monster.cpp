#include "Monster.h"
#include "Bindable/Transform.h"
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
#include "Bindable/SoundBindable.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"

namespace Client
{
	Monster::Monster(int iMaxHP, int iAttackMin, int iAttackMax) :
		m_iInitHP(iMaxHP)
		, m_iInitAttackMin(iAttackMin)
		, m_iInitAttackMax(iAttackMax)
		, m_eState(MONSTER_STATE::IDLE)
	{
	}

	Monster::~Monster()
	{
	}

	bool Monster::SetState(MONSTER_STATE eState)
	{
		switch (m_eState)
		{
		case MONSTER_STATE::IDLE:
			break;
		case MONSTER_STATE::RUN:
			switch (eState)
			{
			case MONSTER_STATE::IDLE:
				break;
			case MONSTER_STATE::RUN:
				return false;
			case MONSTER_STATE::ATTACK:
				break;
			case MONSTER_STATE::END:
				break;
			default:
				break;
			}
			break;
		case MONSTER_STATE::ATTACK:
			break;
		case MONSTER_STATE::HIT:
			switch (eState)
			{
			case MONSTER_STATE::IDLE:
				break;
			case MONSTER_STATE::RUN:
				break;
			case MONSTER_STATE::ATTACK:
				break;
			case MONSTER_STATE::HIT:
				break;
			case MONSTER_STATE::HIT_END:
				break;
			case MONSTER_STATE::END:
				break;
			default:
				break;
			}
			break;
		case MONSTER_STATE::DIE:
			return false;
		case MONSTER_STATE::END:
			break;
		default:
			break;
		}

		m_eState = eState;

		if (!m_pAnimation) return false;

		switch (m_eState)
		{
		case MONSTER_STATE::IDLE:
			m_pAnimation->ChangeSequence("Idle");
			break;
		case MONSTER_STATE::RUN:
			m_pAnimation->ChangeSequence("Run");
			break;
		case MONSTER_STATE::ATTACK:
			m_pAnimation->ChangeSequence("Attack");
			break;
		case MONSTER_STATE::HIT:
			m_pAnimation->ChangeSequence("Jump");
			break;
		case MONSTER_STATE::DIE:
			m_pAnimation->ChangeSequence("Death");
			if (m_pClawBody)
			{
				m_pClawBody->InActivate();
			}
			return false;
		case MONSTER_STATE::END:
			break;
		default:
			break;
		}

		return false;
	}

	bool Monster::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		// Phase E5 — assemble entity from Components.
		m_pTransform     = AddComponent<Engine::Transform>("transform");
		m_pMeshRenderer  = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
		m_pAnimation     = AddComponent<Engine::Animation>("FrogAnimation");
		m_pAttackable    = AddComponent<Attackable>("attackable",
			m_iInitHP, m_iInitAttackMin, m_iInitAttackMax);

		// Mesh + shaders + IL + Topology — install via the static slots
		// directly (this monster's frog mesh is preregistered in scene
		// init; not loaded from .obj here).
		if (m_pMeshRenderer)
		{
			std::shared_ptr<Engine::Mesh> pFrogMesh = Engine::StaticFindBindable<Engine::Mesh>("Idle2");
			if (pFrogMesh)
			{
				pFrogMesh->UsePaperBurn();
				m_pMeshRenderer->SetMesh(pFrogMesh);
			}

			m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_ANIM_VS));
			m_pMeshRenderer->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>(STANDARD_PS));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));

			m_pMeshRenderer->SetAnimation(m_pAnimation);
		}

		if (m_pAnimation)
		{
			m_pAnimation->FindAndAddSequence("Idle");
			m_pAnimation->FindAndAddSequence("Run");
			m_pAnimation->FindAndAddSequence("Attack");

			m_pAnimation->SetLoop("Idle");
			m_pAnimation->SetLoop("Run");

			//m_pAnimation->FindAndAddSequence("FrogArmature|Frog_Death");

			m_pAnimation->SetSkeleton("Idle");

			if (auto pCurr = m_pAnimation->GetCurrentSequence())
				pCurr->Loop();
		}

		if (m_pTransform)
		{
			m_pTransform->SetScale(0.01f, 0.01f, 0.01f);
			m_pTransform->SetPosition(5.f, 127.f, 5.f);
		}

		// Phase E5 — Terrain is a GameObject now; look it up in the layer.
		std::shared_ptr<Engine::Layer> pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
		std::shared_ptr<Engine::GameObject> pTerrainObj =
			pLayer ? pLayer->FindGameObject("NavMesh") : nullptr;

		if (pTerrainObj)
		{
			std::shared_ptr<Engine::NavMesh> pNavMesh =
				std::static_pointer_cast<Engine::NavMesh>(pTerrainObj->FindComponent(Engine::COMPONENT_TYPE::NAV_MESH));

			if (pNavMesh && m_pTransform)
			{
				m_pAgent = pNavMesh->CreateAgent(GetTag() + "agent", m_pTransform, m_pTransform->GetPosition());

				AddComponent(m_pAgent);
			}
		}

		m_pBody = AddComponent<Engine::ColliderSphere>(GetTag() + "body");
		if (m_pBody)
		{
			m_pBody->SetRadius(0.5f);
			m_pBody->SetOffset({ 0.f, 0.25f, 0.f });
			m_pBody->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Monster::CollisionEnter);
		}

		// Phase E5 — Claw is now a GameObject. JointSocket holds the
		// claw's Transform Component directly via the new AddSocket
		// GameObject overload, so the claw doesn't have to be a Drawable.
		std::shared_ptr<Engine::GameObject> pClaw =
			GetScene()->CreateGameObject<>("Claw", GetScene()->FindLayer(DEFAULT_LAYER));
		pClaw->AddComponent<Engine::Transform>("transform");
		pClaw->AddComponent<Attackable>("attackable", 30, 5, 10);

		m_pClawBody = pClaw->AddComponent<Engine::ColliderOBB>(GetTag() + "clawbody");

		m_pClawBody->SetScaleOffset({ 0.2f, 0.1f, 0.2f });

		if (m_pAnimation)
		{
			std::shared_ptr<Engine::JointSocket> pSocket = m_pAnimation->AddSocket(5, pClaw);
			if (pSocket) pSocket->SetPosition({ 0.f, 0.f, 1.f });

			if (std::shared_ptr<Engine::Notify> pDieNotify = m_pAnimation->AddNotify("FrogArmature|Frog_Death", "PaperBurn", 0.8f))
			{
				pDieNotify->SetCallBack(
					[this](int, float, Engine::Bindable*)
					{
						if (m_pAttackable)
						{
							m_pAttackable->StartPaperBurn();
							if (auto pParticle = m_pAttackable->GetParticle())
								pParticle->StopEmit();
						}
					}
				);
			}
		}

		m_pAttackSound = pClaw->AddComponent<Engine::SoundBindable>("attack sound", "melee sound");

		if (m_pAnimation)
		{
			if (std::shared_ptr<Engine::Notify> pAttackNotify = m_pAnimation->AddNotify("FrogArmature|Frog_Attack", "Attack", 0.5f))
			{
				pAttackNotify->SetCallBack(
					[this](int, float, Engine::Bindable*)
					{
						if (m_pAttackSound) m_pAttackSound->Play();
					}
				);
			}
		}

		return true;
	}

	void Monster::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		// Phase E5 — Player is a GameObject now; lookup via the layer's
		// FindGameObject and use Player's GetTransform helper.
		std::shared_ptr<Engine::Layer> pPlayerLayer = GetScene()->FindLayer(DEFAULT_LAYER);
		std::shared_ptr<Engine::GameObject> pPlayer =
			pPlayerLayer ? pPlayerLayer->FindGameObject("player") : nullptr;

		if (pPlayer && m_pTransform)
		{
			std::shared_ptr<Engine::Transform> pPlayerTransform =
				pPlayer->GetComponent<Engine::Transform>();
			if (!pPlayerTransform) return;

			const Engine::Vector3& vPlayerPos = pPlayerTransform->GetPosition();

			const Engine::Vector3& vPos = m_pTransform->GetPosition();

			float fTargetDist = (vPos - vPlayerPos).Length();

			if (fTargetDist < 1.f)
			{
				SetState(MONSTER_STATE::ATTACK);

				if (m_pAgent) m_pAgent->SetTargetPos(vPos);
			}
			else if (fTargetDist < 10.f)
			{
				SetState(MONSTER_STATE::RUN);

				if (m_pAgent) m_pAgent->SetTargetPos(vPlayerPos);
			}
			else
			{
				SetState(MONSTER_STATE::IDLE);

				if (m_pAgent) m_pAgent->SetTargetPos(vPos);
			}
		}
		else
		{
			SetState(MONSTER_STATE::IDLE);
		}

		switch (m_eState)
		{
		case MONSTER_STATE::IDLE:
			break;
		case MONSTER_STATE::RUN:
			break;
		case MONSTER_STATE::ATTACK:
			break;
		case MONSTER_STATE::HIT:
		{
		}
		break;
		case MONSTER_STATE::HIT_END:
			break;
		case MONSTER_STATE::END:
			break;
		default:
			break;
		}
	}

	void Monster::CollisionEnter(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (pDest->GetTag() == "sword_body")
		{
			// Phase E5 — attacker is GameObject-hosted (Drawable hosts gone).
			std::shared_ptr<Attackable> pWeapon;
			if (Engine::GameObject* pAttackerOwner = pDest->GetGameObjectOwner())
				pWeapon = pAttackerOwner->GetComponent<Attackable>();

			if (pWeapon && pWeapon->Attack(m_pAttackable.get()))
			{
				SetState(MONSTER_STATE::DIE);

				if (m_pAgent) m_pAgent->InActivate();
			}
			else
			{
				SetState(MONSTER_STATE::HIT);
			}
		}
	}
}

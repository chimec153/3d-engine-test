#include "Monster.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT_EX(Monster, new Client::Monster(50, 1, 3))
#include "MonsterState.h"
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
	{
	}

	Monster::~Monster()
	{
	}

	void Monster::ChangeState(std::unique_ptr<IMonsterState> pNext)
	{
		if (!pNext) return;

		// Delegate Exit/Enter sequencing to the generic state machine,
		// then layer the animation sync on top so each individual state's
		// Enter doesn't have to know about Animation.
		m_stateMachine.ChangeState(std::move(pNext));

		if (m_pAnimation)
		{
			const char* pSeq = m_stateMachine.GetCurrentAnimSequence();
			if (pSeq && pSeq[0])
				m_pAnimation->ChangeSequence(pSeq);
		}
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
			if (auto pRunSequance = m_pAnimation->FindAndAddSequence("Run"))
			{
				pRunSequance->SetNextSequence("Idle");
			}
			if (auto pAttackSequence = m_pAnimation->FindAndAddSequence("Attack"))
			{
				pAttackSequence->SetNextSequence("Idle");
			}

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
			if (std::shared_ptr<Engine::Notify> pAttackNotify = m_pAnimation->AddNotify("Attack", "Attack", 0.5f))
			{
				pAttackNotify->SetCallBack(
					[this](int, float, Engine::Bindable*)
					{
						if (m_pAttackSound) m_pAttackSound->Play();
					}
				);
			}

			// "AttackEnd" — fires near the tail of the Attack clip. Sets a
			// flag that MonsterAttackState::Update polls (and consumes) to
			// decide when it's safe to leave the locked Attack state.
			if (std::shared_ptr<Engine::Notify> pAttackEnd = m_pAnimation->AddNotify("Attack", "AttackEnd", 0.95f))
			{
				pAttackEnd->SetCallBack(
					[this](int, float, Engine::Bindable*)
					{
						m_bAttackAnimFinished = true;
					}
				);
			}
		}

		// Initial state. MonsterIdleState::Enter parks the agent so we
		// don't drift before the first Update tick.
		ChangeState(std::make_unique<MonsterIdleState>());

		return true;
	}

	void Monster::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		// Dispatch through Monster::ChangeState (not m_stateMachine.Update)
		// so the animation sync in ChangeState runs on every transition.
		if (auto* pCurr = m_stateMachine.GetCurrent())
		{
			if (auto pNext = pCurr->Update(*this, fDeltaTime))
				ChangeState(std::move(pNext));
		}
	}

	void Monster::CollisionEnter(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (pDest->GetTag() != "sword_body") return;

		// Phase E5 — attacker is GameObject-hosted (Drawable hosts gone).
		std::shared_ptr<Attackable> pWeapon;
		if (Engine::GameObject* pAttackerOwner = pDest->GetGameObjectOwner())
			pWeapon = pAttackerOwner->GetComponent<Attackable>();
		if (!pWeapon) return;

		// Attack returns true when the hit was lethal.
		const bool bLethal = pWeapon->Attack(m_pAttackable.get());

		// Let the current state decide first — Die overrides hit-reaction
		// by returning nullptr, for example.
		if (auto* pCurr = m_stateMachine.GetCurrent())
		{
			if (auto pNext = pCurr->OnHit(*this, bLethal))
			{
				ChangeState(std::move(pNext));
				return;
			}
		}

		// Default reaction when the active state doesn't override.
		if (bLethal)
			ChangeState(std::make_unique<MonsterDieState>());
		else
			ChangeState(std::make_unique<MonsterHitState>());
	}
}

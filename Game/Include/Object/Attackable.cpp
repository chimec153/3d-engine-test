#include "Attackable.h"
#include "Bindable/PaperBurn.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Particle.h"
#include "Bindable/Texture.h"
#include "Bindable/SoundBindable.h"
#include "Bindable/Transform.h"
#include "GameObject/GameObject.h"

namespace
{
	// Phase E5 — Drawable hosts gone; this helper is now just a thin
	// pass-through to GameObject::AddComponent. Kept named the same for
	// minimal change to the call sites below.
	template <typename T, typename ...Args>
	std::shared_ptr<T> CreateSiblingComponent(Engine::GameObject* pOwnerGameObject,
		const std::string& strTag, Args... args)
	{
		if (pOwnerGameObject)
			return pOwnerGameObject->AddComponent<T>(strTag, args...);
		return nullptr;
	}
}

namespace Client
{
	Attackable::Attackable()	:
		m_iMaxHP(0)
		, m_iHP(0)
		, m_iAttackMin(0)
		, m_iAttackMax(0)
	{
		SetComponentType(Engine::COMPONENT_TYPE::NONE);
	}

	Attackable::Attackable(int iMaxHP, int iAttackMin, int iAttackMax, bool bWithBloodParticle, bool bWithPaperBurn)	:
		m_iMaxHP(iMaxHP)
		, m_iHP(iMaxHP)
		, m_iAttackMin(iAttackMin)
		, m_iAttackMax(iAttackMax)
		, m_bWithBloodParticle(bWithBloodParticle)
		, m_bWithPaperBurn(bWithPaperBurn)
	{
		SetComponentType(Engine::COMPONENT_TYPE::NONE);
	}

	Attackable::Attackable(const Attackable& other)	:
		Engine::Component(other)
		, m_iMaxHP(other.m_iMaxHP)
		, m_iHP(other.m_iHP)
		, m_iAttackMin(other.m_iAttackMin)
		, m_iAttackMax(other.m_iAttackMax)
		, m_bWithBloodParticle(other.m_bWithBloodParticle)
		, m_bWithPaperBurn(other.m_bWithPaperBurn)
		, m_pPaperBurn(other.m_pPaperBurn)
		, m_pParticle(other.m_pParticle)
		, m_pBloodParticle(other.m_pBloodParticle)
		, m_pHitSound(other.m_pHitSound)
	{
	}

	bool Attackable::Attack(Attackable* pTargetAttackable) const
	{
		if (!pTargetAttackable) return false;

		int iAttack = GetAttack();

		if (m_pBloodParticle) m_pBloodParticle->AddEmitCount(iAttack * 64);
		if (m_pHitSound)      m_pHitSound->Play();

		return (pTargetAttackable->m_iHP -= iAttack) <= 0;
	}

	int Attackable::GetAttack() const
	{
		return m_iAttackMin + static_cast<int>((m_iAttackMax - m_iAttackMin) * (rand() / static_cast<float>(RAND_MAX)));
	}

	void Attackable::StartPaperBurn()
	{
		if (!m_pPaperBurn) return;
		m_pPaperBurn->StartPaperBurn();
	}

	bool Attackable::Init()
	{
		if (!__super::Init())
			return false;

		// Phase E5 — Attackable installs PaperBurn / Particle /
		// SoundBindable as sibling Components on the owning GameObject.
		// (Drawable hosts no longer exist live.)
		Engine::GameObject* pOwnerGameObject = GetGameObjectOwner();
		if (!pOwnerGameObject) return true;

		// Opt-out: enemies dissolve per-instance via EnemyMeshRenderer's
		// instance stream and must not carry a PaperBurn sibling (it would
		// disqualify their bucket from the DrawInstanced fast path).
		if (m_bWithPaperBurn)
			m_pPaperBurn = CreateSiblingComponent<Engine::PaperBurn>(
				pOwnerGameObject,
				"paperburn", Engine::StaticFindBindable<Engine::Texture>("PaperBurn"));

		if (m_pPaperBurn)
		{
			m_pPaperBurn->SetMaxTime(4.f);

			m_pPaperBurn->SetStartRate(0.1f);
			m_pPaperBurn->SetMidRate(0.2f);
			m_pPaperBurn->SetFinalRate(0.75f);

			m_pPaperBurn->SetStartColor(Engine::Red);
			m_pPaperBurn->SetMidColor(Engine::Yellow);
			m_pPaperBurn->SetFinalColor(Engine::White);

			m_pPaperBurn->AddCallBack(Engine::PaperBurn::PAPER_BURN_STAGE::OUT_STAGE, [this](float)
				{
					//InActivate();
				}
			);

			m_pPaperBurn->AddCallBack(Engine::PaperBurn::PAPER_BURN_STAGE::MID, [this](float)
				{
					if (m_pParticle) m_pParticle->ResumeEmit();
				}
			);

			m_pPaperBurn->AddCallBack(Engine::PaperBurn::PAPER_BURN_STAGE::FINAL, [this](float)
				{
					if (m_pParticle) m_pParticle->StopEmit();
				}
			);
		}

		std::shared_ptr<Engine::Texture> pParticleTex =
			Engine::StaticCreateBindable<Engine::Texture>("particletexture", "/Game/Texture/Particle/particle_00.png", TEXTURE_PATH);

		m_pParticle = CreateSiblingComponent<Engine::Particle>(
			pOwnerGameObject, "particle", 4096);

		if (m_pParticle)
		{
			m_pParticle->SetAccelaration({ 0.f, 1.f, 0.f });
			m_pParticle->SetEmitTime(0.001f);
			m_pParticle->SetMinCreatePosition({ -1.f, 0.f, -1.f });
			m_pParticle->SetMaxCreatePosition({ 1.f, 0.f, 1.f });
			m_pParticle->SetStartColor(Engine::Yellow);
			m_pParticle->SetEndColor({ 1.f, 1.f, 0.f, 0.f });
			m_pParticle->SetMaxLifeTime(3.f);
			m_pParticle->SetStartSize({ 0.02f, 0.02f });
			m_pParticle->SetEndSize({ 0.1f, 0.1f });
			m_pParticle->SetVelocity({ -0.1f, 0.f, -0.1f });
			m_pParticle->SetMaxVelocity({ 0.1f, 0.f, 0.1f });
			m_pParticle->SetTexture(pParticleTex);
			m_pParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
			m_pParticle->StopEmit();
		}

		// Blood particle is opt-in (see Attackable.h constructor doc).
		// Every Attackable that opts in pays a per-frame CS dispatch +
		// system-buffer upload, which dominated the profile when enemies
		// each had one. Default-off means enemies / monsters skip it
		// entirely; only entities that pass bWithBloodParticle=true (the
		// player) get the emitter.
		if (m_bWithBloodParticle)
		{
			m_pBloodParticle = CreateSiblingComponent<Engine::Particle>(
				pOwnerGameObject, "bloodparticle", 512);

			if (m_pBloodParticle)
			{
				m_pBloodParticle->SetAccelaration({ 0.f, -3.f, 0.f });
				m_pBloodParticle->SetEmitTime(0.0001f);
				m_pBloodParticle->SetStartColor({ 0.4f, 0.f, 0.f, 1.0f });
				m_pBloodParticle->SetEndColor({ 0.4f, 0.f, 0.f, 0.9f });
				m_pBloodParticle->SetMaxLifeTime(1.f);
				m_pBloodParticle->SetStartSize({ 0.04f, 0.04f });
				m_pBloodParticle->SetEndSize({ 0.04f, 0.04f });
				m_pBloodParticle->SetVelocity({ -0.2f, -0.2f, -0.2f });
				m_pBloodParticle->SetMaxVelocity({ 0.2f, 0.2f, 0.2f });
				m_pBloodParticle->SetTexture(pParticleTex);
				m_pBloodParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
				m_pBloodParticle->AddEmitCount(1);
			}
		}

		char strHit[TEXT_LEN] = {};
		sprintf_s(strHit, "hit%02d", rand() % 38);

		m_pHitSound = CreateSiblingComponent<Engine::SoundBindable>(
			pOwnerGameObject, "hitsound", strHit);

		return true;
	}

	void Attackable::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		// Each sibling Particle component owns a standalone Transform that
		// Particle::Init seeded at (0,0,0) and that nothing else writes to.
		// Without per-frame sync, particles spawn at world origin instead
		// of the host's position — Bullet uses the same pattern (Bullet
		// updates its trail's Transform every Update). Mirror that here
		// for the yellow/blood emitters created in Init.
		Engine::GameObject* pOwner = GetGameObjectOwner();
		if (!pOwner) return;
		auto pHostTr = pOwner->GetComponent<Engine::Transform>();
		if (!pHostTr) return;

		const Engine::Vector3& vPos = pHostTr->GetPosition();
		const Engine::Vector3& vRot = pHostTr->GetRotation();

		if (m_pParticle)
		{
			if (auto pTr = m_pParticle->GetTransform())
			{
				pTr->SetPosition(vPos);
				pTr->SetRotation(vRot);
			}
		}
		if (m_pBloodParticle)
		{
			if (auto pTr = m_pBloodParticle->GetTransform())
			{
				pTr->SetPosition(vPos);
				pTr->SetRotation(vRot);
			}
		}
	}

	std::shared_ptr<Engine::Component> Attackable::Clone()
	{
		return std::make_shared<Attackable>(*this);
	}
}

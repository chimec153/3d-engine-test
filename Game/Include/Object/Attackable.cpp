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

	Attackable::Attackable(int iMaxHP, int iAttackMin, int iAttackMax, bool bWithBloodParticle, bool bWithPaperBurn, bool bWithImpactBurst)	:
		m_iMaxHP(iMaxHP)
		, m_iHP(iMaxHP)
		, m_iAttackMin(iAttackMin)
		, m_iAttackMax(iAttackMax)
		, m_bWithBloodParticle(bWithBloodParticle)
		, m_bWithPaperBurn(bWithPaperBurn)
		, m_bWithImpactBurst(bWithImpactBurst)
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
		, m_bWithImpactBurst(other.m_bWithImpactBurst)
		, m_pPaperBurn(other.m_pPaperBurn)
		, m_pParticle(other.m_pParticle)
		, m_pBloodParticle(other.m_pBloodParticle)
		, m_pDustParticle(other.m_pDustParticle)
		, m_pImpactParticle(other.m_pImpactParticle)
		, m_pHitSound(other.m_pHitSound)
	{
	}

	bool Attackable::Attack(Attackable* pTargetAttackable) const
	{
		if (!pTargetAttackable) return false;

		int iAttack = GetAttack();

		// Blood spurts from the entity taking the hit, so emit on the
		// TARGET's blood particle (the player), not the attacker's — the
		// attacker (enemy) has none. Same private-member access works
		// because Attack is a member of Attackable.
		if (pTargetAttackable->m_pBloodParticle) pTargetAttackable->m_pBloodParticle->AddEmitCount(iAttack * 64);
		// Grey impact puff alongside the blood — fixed burst so it reads as
		// a consistent "hit dust" regardless of the damage rolled.
		if (pTargetAttackable->m_pDustParticle)  pTargetAttackable->m_pDustParticle->AddEmitCount(40);
		// Animated impact flash — a small cluster whose flipbook plays once.
		if (pTargetAttackable->m_pImpactParticle) pTargetAttackable->m_pImpactParticle->AddEmitCount(6);
		if (m_pHitSound)      m_pHitSound->Play();

		// Apply the TARGET's damage reduction (the player's "defense up" stat).
		int iDmg = iAttack;
		const float fRed = pTargetAttackable->m_fDamageReduction;
		if (fRed > 0.f)
			iDmg = static_cast<int>(iAttack * (1.f - fRed) + 0.5f);
		if (iDmg < 0) iDmg = 0;

		return (pTargetAttackable->m_iHP -= iDmg) <= 0;
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

			// Grey dust puff that accompanies the blood: bigger, slower,
			// fading particles that drift up-and-out so the hit kicks up
			// a small cloud. Reuses the same soft round particle texture,
			// just tinted grey. Same arm pattern as blood (AddEmitCount(1)
			// moves the emit count from -1/continuous to 0/burst-on-demand).
			m_pDustParticle = CreateSiblingComponent<Engine::Particle>(
				pOwnerGameObject, "dustparticle", 256);

			if (m_pDustParticle)
			{
				m_pDustParticle->SetAccelaration({ 0.f, 0.4f, 0.f });
				m_pDustParticle->SetEmitTime(0.0001f);
				m_pDustParticle->SetStartColor({ 0.55f, 0.52f, 0.47f, 0.6f });
				m_pDustParticle->SetEndColor({ 0.6f, 0.57f, 0.52f, 0.f });
				m_pDustParticle->SetMaxLifeTime(0.6f);
				m_pDustParticle->SetStartSize({ 0.06f, 0.06f });
				m_pDustParticle->SetEndSize({ 0.22f, 0.22f });
				m_pDustParticle->SetVelocity({ -0.5f, 0.f, -0.5f });
				m_pDustParticle->SetMaxVelocity({ 0.5f, 0.4f, 0.5f });
				m_pDustParticle->SetTexture(pParticleTex);
				m_pDustParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
				m_pDustParticle->AddEmitCount(1);
			}
		}

		// Animated impact flash. Square 4x4 flipbook (16 frames): the GS
		// advances the frame by particle age, so the texture itself carries
		// the burst/expansion — size and velocity stay ~fixed. MUST be a
		// square N×N atlas (FrameWidth==FrameHeight==N), see the GS row calc
		// (/FrameHeight). Tinted white. Opt-in via bWithImpactBurst so the
		// player AND towers (which have no blood) show a hit flash.
		if (m_bWithImpactBurst)
		{
			std::shared_ptr<Engine::Texture> pImpactTex =
				Engine::StaticCreateBindable<Engine::Texture>("hitimpacttexture", "/Game/Texture/Particle/burst.png", TEXTURE_PATH);

			m_pImpactParticle = CreateSiblingComponent<Engine::Particle>(
				pOwnerGameObject, "impactparticle", 64);

			if (m_pImpactParticle)
			{
				m_pImpactParticle->SetFrameWidth(4);
				m_pImpactParticle->SetFrameHeight(4);
				m_pImpactParticle->SetMaxFrame(16);
				m_pImpactParticle->SetAccelaration({ 0.f, 0.f, 0.f });
				m_pImpactParticle->SetEmitTime(0.001f);
				m_pImpactParticle->SetStartColor({ 1.f, 1.f, 1.f, 1.f });
				m_pImpactParticle->SetEndColor({ 1.f, 1.f, 1.f, 1.f });
				// The bright frames are only the FIRST half of the sheet, so
				// the quad must already be large when the burst spawns —
				// growing from small made the bright moment tiny. Start big
				// (≈2-3x a tower's ~0.9-unit width so a hit is unmissable) and
				// only grow a little. 0.8s life keeps the flash readable.
				m_pImpactParticle->SetMaxLifeTime(0.8f);
				float fSize = 0.5f;
				m_pImpactParticle->SetStartSize({ fSize, fSize });
				m_pImpactParticle->SetEndSize({ fSize, fSize });
				float fPosition = 0.5f;
				m_pImpactParticle->SetMinCreatePosition({ -fPosition, 0.5f, -fPosition });
				m_pImpactParticle->SetMaxCreatePosition({ fPosition, 1.f, fPosition });
				m_pImpactParticle->SetVelocity({ 0.f, 0.f, 0.f });
				m_pImpactParticle->SetMaxVelocity({ 0.f, 0.f, 0.f });
				m_pImpactParticle->SetTexture(pImpactTex);
				m_pImpactParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
				m_pImpactParticle->AddEmitCount(1);
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
		if (m_pDustParticle)
		{
			if (auto pTr = m_pDustParticle->GetTransform())
			{
				pTr->SetPosition(vPos);
				pTr->SetRotation(vRot);
			}
		}
		if (m_pImpactParticle)
		{
			if (auto pTr = m_pImpactParticle->GetTransform())
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

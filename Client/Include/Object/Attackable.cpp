#include "Attackable.h"
#include "Bindable/PaperBurn.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Particle.h"
#include "Bindable/Decal.h"
#include "Scene/Scene.h"
#include "Bindable/Transform.h"
#include "Bindable/SoundBindable.h"

Client::Attackable::Attackable(int iMaxHP, int iAttackMin, int iAttackMax)	:
	Drawable()
	, m_iMaxHP(iMaxHP)
	, m_iHP(iMaxHP)
	, m_iAttackMin(iAttackMin)
	, m_iAttackMax(iAttackMax)
{
}

bool Client::Attackable::Attack(Attackable* pHit) const
{
	std::shared_ptr<Engine::Decal> pDecal = std::static_pointer_cast<Engine::Decal>(GetScene()->CreateCloneDrawable("blooddecal", "blooddecal", GetScene()->FindLayer(DEFAULT_LAYER)));

	std::shared_ptr<Engine::Transform> pDecalTransform = pDecal->GetTransform();

	pDecalTransform->SetPosition(GetTransform()->GetPosition());

	pDecalTransform->SetRY(GetTransform()->GetRY());

	int iAttack = GetAttack();

	float fRate = iAttack / static_cast<float>(m_iMaxHP);

	pDecal->SetFadeStartTime(20.f * fRate);

	m_pBloodParticle->AddEmitCount(iAttack * 64);

	m_pHitSound->Play();

	return (pHit->m_iHP -= iAttack) <= 0;
}

int Client::Attackable::GetAttack() const
{
	return m_iAttackMin + static_cast<int>((m_iAttackMax - m_iAttackMin) * (rand() / static_cast<float>(RAND_MAX)));
}

void Client::Attackable::StartPaperBurn()
{
	if (!m_pPaperBurn)
	{
		return;
	}

	m_pPaperBurn->StartPaperBurn();
}

std::shared_ptr<Engine::PaperBurn> Client::Attackable::GetPaperBurn() const
{
	return m_pPaperBurn;
}

std::shared_ptr<Engine::Particle> Client::Attackable::GetParticle() const
{
	return m_pParticle;
}

bool Client::Attackable::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	m_pPaperBurn = CreateBindable<Engine::PaperBurn>("paperburn", Engine::StaticFindBindable<Engine::Texture>("PaperBurn"));

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
			m_pParticle->ResumeEmit();
		}
	);

	m_pPaperBurn->AddCallBack(Engine::PaperBurn::PAPER_BURN_STAGE::FINAL, [this](float)
		{
			m_pParticle->StopEmit();
		}
	);

	m_pParticle = CreateBindable<Engine::Particle>("particle", 4096);

	m_pParticle->SetAccelaration({ 0.f, 1.f, 0.f });
	m_pParticle->SetEmitTime(0.001f);
	m_pParticle->SetMinCreatePosition({ -1.f, 0.f, -1.f });
	m_pParticle->SetMaxCreatePosition({ 1.f, 0.f, 1.f });
	m_pParticle->SetStartColor(Engine::Yellow);
	m_pParticle->SetEndColor({1.f, 1.f, 0.f, 0.f});
	m_pParticle->SetMaxLifeTime(3.f);
	m_pParticle->SetStartSize({0.02f, 0.02f });
	m_pParticle->SetEndSize({ 0.1f, 0.1f });
	m_pParticle->SetVelocity({-0.1f, 0.f, -0.1f });
	m_pParticle->SetMaxVelocity({ 0.1f, 0.f, 0.1f });
	m_pParticle->CreateBindable<Engine::Texture>("particletexture", "Particle\\particle_00.png", TEXTURE_PATH);
	m_pParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
	m_pParticle->StopEmit();


	m_pBloodParticle = CreateBindable<Engine::Particle>("particle", 512);

	m_pBloodParticle->SetAccelaration({ 0.f, -3.f, 0.f });
	m_pBloodParticle->SetEmitTime(0.0001f);
	//m_pBloodParticle->SetMinCreatePosition({ -0.1f, -0.1f, -0.1f });
	//m_pBloodParticle->SetMaxCreatePosition({ 0.1f, 0.1f, 0.1f });
	m_pBloodParticle->SetStartColor({ 0.4f, 0.f, 0.f, 1.0f });
	m_pBloodParticle->SetEndColor({ 0.4f, 0.f, 0.f, 0.9f });
	m_pBloodParticle->SetMaxLifeTime(1.f);
	m_pBloodParticle->SetStartSize({ 0.04f, 0.04f });
	m_pBloodParticle->SetEndSize({ 0.04f, 0.04f });
	m_pBloodParticle->SetVelocity({ -0.2f, -0.2f, -0.2f });
	m_pBloodParticle->SetMaxVelocity({ 0.2f, 0.2f, 0.2f });
	m_pBloodParticle->CreateBindable<Engine::Texture>("particletexture", "Particle\\particle_00.png", TEXTURE_PATH);
	m_pBloodParticle->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
	m_pBloodParticle->AddEmitCount(1);

	char strHit[TEXT_LEN] = {};

	sprintf_s(strHit, "hit%02d", rand() % 38);

	m_pHitSound = CreateBindable<Engine::SoundBindable>("hitsound", strHit);

	return true;
}

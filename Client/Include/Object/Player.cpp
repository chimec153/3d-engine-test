#include "Player.h"
#include "Core/Graphics.h"
#include "Bindable/Camera.h"
#include "Input/Input.h"
#include "Bindable/TransformBuffer.h"
#include "Bindable/Animation.h"
#include "Resource/ResourceManager.h"
#include "Animation/Sequence.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/PixelShader.h"
#include "Bindable/VertexShader.h"
#include "Scene/Scene.h"
#include "Bindable/Terrain.h"

Client::Player::Player() :
	Engine::Drawable()
	, m_fSpeed(25.f)
{
}

void Client::Player::Start()
{
	__super::Start();

	std::shared_ptr<Engine::Layer> pLayer = GetScene()->FindLayer(DEFAULT_LAYER);

	m_pTerrain = std::static_pointer_cast<Engine::Terrain>(pLayer->FindDrawable(Engine::BINDABLE_TYPE::TERRAIN));
}

bool Client::Player::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	CreateBindable<Engine::Mesh>("PlayerMesh", "Medieval.mesh");
	FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
	FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
	FindAndAddBind<Engine::InputLayout>("Standard");
	FindAndAddBind<Engine::Topology>("TriangleList");

	std::shared_ptr<Engine::Animation> pAnimation = CreateBindable<Engine::Animation>("PlayerAnimation");

	std::vector<std::string> vecSeq = {
		"CharacterArmature|Death",
		"CharacterArmature|Gun_Shoot",
		"CharacterArmature|HitRecieve",
		"CharacterArmature|HitRecieve_2",
		"CharacterArmature|Idle",
		"CharacterArmature|Idle_Gun",
		"CharacterArmature|Idle_Gun_Pointing",
		"CharacterArmature|Idle_Gun_Shoot",
		"CharacterArmature|Idle_Neutral",
		"CharacterArmature|Idle_Sword",
		"CharacterArmature|Interact",
		"CharacterArmature|Kick_Left",
		"CharacterArmature|Kick_Right",
		"CharacterArmature|Punch_Left",
		"CharacterArmature|Punch_Right",
		"CharacterArmature|Roll",
		"CharacterArmature|Run",
		"CharacterArmature|Run_Back",
		"CharacterArmature|Run_Left",
		"CharacterArmature|Run_Right",
		"CharacterArmature|Run_Shoot",
		"CharacterArmature|Sword_Slash",
		"CharacterArmature|Walk",
		"CharacterArmature|Wave",
	};

	for (size_t i = 0; i < vecSeq.size(); ++i)
	{
		std::shared_ptr<Engine::Sequence> pSequence = Engine::ResourceManager::GetInst()->FindSequence(vecSeq[i]);

		pAnimation->AddSequance(pSequence->GetTag(), pSequence);
	}

	std::shared_ptr<Engine::Skeleton> pSkeleton = Engine::ResourceManager::GetInst()->FindSkeleton("Medieval");

	assert(pSkeleton);

	pAnimation->SetSkeleton(pSkeleton);

	pAnimation->ChangeSequence("CharacterArmature|Idle");

	m_pCamera = Engine::Graphics::GetInst()->GetCamera();

	AddChild(m_pCamera);

	float fHeight = 4.f;

	float fAngle = PI / 6.f;

	m_pCamera->GetTransform()->SetRelativePosition(0.f, 0.8f + fHeight, -fHeight / tanf(fAngle));

	m_pCamera->GetTransform()->SetRelativeRotation(fAngle, 0.f, 0.f);

	GetTransform()->SetY(10.f);

	return true;
}

void Client::Player::Input(float fDeltaTime)
{
	const std::shared_ptr<Engine::Transform>& pTransform = GetTransform();

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_W))
	{
		pTransform->AddPosition(GetTransform()->GetAxis(Engine::AXIS_TYPE::Z) * fDeltaTime * m_fSpeed);
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_S))
	{
		pTransform->AddPosition(GetTransform()->GetAxis(Engine::AXIS_TYPE::Z) * fDeltaTime * -m_fSpeed);
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_D))
	{
		pTransform->AddPosition(GetTransform()->GetAxis(Engine::AXIS_TYPE::X) * fDeltaTime * m_fSpeed);
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_A))
	{
		pTransform->AddPosition(GetTransform()->GetAxis(Engine::AXIS_TYPE::X) * fDeltaTime * -m_fSpeed);
	}

	if (!Engine::Window::GetInst()->IsLockRotation())
	{
		int iDeltaX = Engine::CInput::GetInst()->GetMouseDeltaX();
		int iDeltaY = Engine::CInput::GetInst()->GetMouseDeltaY();

		pTransform->AddRY(iDeltaX * fDeltaTime);
		m_pCamera->GetTransform()->AddRX(iDeltaY * fDeltaTime);

		if (pTransform->GetRX() >= PI)
		{
			pTransform->SetRX(PI);
		}

		else if (pTransform->GetRX() <= -PI)
		{
			pTransform->SetRX(-PI);
		}
	}

	void Player::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		m_pTerrain;
	}
}

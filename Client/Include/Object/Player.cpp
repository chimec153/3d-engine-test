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
#include "Bindable/Mesh.h"
#include "Bindable/BindableManager.h"
#include "Bindable/DepthStencilState.h"
#include "Animation/JointSocket.h"
#include "Trail.h"
#include "Bindable/ColliderOBB.h"
#include "Bindable/Particle.h"
#include "Attackable.h"
#include "Bindable/Decal.h"
#include "Bindable/ColliderLine.h"

Client::Player::Player(int iMaxHP, int iAttackMin, int iAttackMax) :
	Attackable(iMaxHP, iAttackMin, iAttackMax)
	, m_fSpeed(5.f)
	, m_fAccel(-9.8f)
	, m_fFallSpeed(0.f)
	, m_fRollSpeed(7.f)
	, m_eState(PLAYER_STATE::IDLE)
	, m_eUpperState(PLAYER_UPPER_BODY_STATE::IDLE)
	, m_iMaxShadowFrame(15)
	, m_fCameraDist(10.f)
	, m_bCanJump(true)
{
}

bool Client::Player::SetState(PLAYER_STATE eState)
{
	switch (m_eState)
	{
	case Client::Player::PLAYER_STATE::IDLE:
		break;
	case Client::Player::PLAYER_STATE::RUN:
		switch (eState)
		{
		case Client::Player::PLAYER_STATE::IDLE:
			break;
		case Client::Player::PLAYER_STATE::RUN:
			break;
		case Client::Player::PLAYER_STATE::ROLL:
			break;
		case Client::Player::PLAYER_STATE::DIE:
			break;
		case Client::Player::PLAYER_STATE::END:
			break;
		default:
			break;
		}
		break;
	case Client::Player::PLAYER_STATE::ROLL:
		switch (eState)
		{
		case Client::Player::PLAYER_STATE::ROLL_END:
		case Client::Player::PLAYER_STATE::DIE:
			m_pBody->Enable();
			break;
		default:
			return false;
		}
		break;
	case Client::Player::PLAYER_STATE::HIT:
	{
		switch (eState)
		{
		case Client::Player::PLAYER_STATE::HIT_END:
		case Client::Player::PLAYER_STATE::DIE:
			break;
		default:
			return false;
		}
	}
		break;
	case Client::Player::PLAYER_STATE::HIT_END:
		break;
	case Client::Player::PLAYER_STATE::DIE:
		return false;
	}

	m_eState = eState;

	switch (eState)
	{
	case Client::Player::PLAYER_STATE::IDLE:
		ChangeSequence("CharacterArmature|Idle");
		break;
	case Client::Player::PLAYER_STATE::RUN:

		switch (m_eDir)
		{
		case Client::Player::MOVE_DIR::LEFT:
			ChangeSequence("CharacterArmature|Run_Left");
			break;
		case Client::Player::MOVE_DIR::RIGHT:
			ChangeSequence("CharacterArmature|Run_Right");
			break;
		case Client::Player::MOVE_DIR::UP:
			ChangeSequence("CharacterArmature|Run");
			break;
		case Client::Player::MOVE_DIR::DOWN:
			ChangeSequence("CharacterArmature|Run_Back");
			break;
		case Client::Player::MOVE_DIR::END:
			break;
		default:
			break;
		}
		break;
	case Client::Player::PLAYER_STATE::ROLL:
		m_pBody->Disable();
		SetRate(2.f);
		ChangeSequence("CharacterArmature|Roll");
		break;
	case Client::Player::PLAYER_STATE::ROLL_END:
		SetRate(1.f);
		break;
	case Client::Player::PLAYER_STATE::HIT:
		ChangeSequence("CharacterArmature|HitRecieve");
		break;
	case Client::Player::PLAYER_STATE::DIE:
		ChangeSequence("CharacterArmature|Death");
		break;
	}

	return true;
}

bool Client::Player::SetUpperBodyState(PLAYER_UPPER_BODY_STATE eState)
{
	switch (m_eState)
	{
	case Client::Player::PLAYER_STATE::IDLE:
		break;
	case Client::Player::PLAYER_STATE::RUN:
		break;
	case Client::Player::PLAYER_STATE::ROLL:
	{
		switch (eState)
		{
		case Client::Player::PLAYER_UPPER_BODY_STATE::IDLE:
			break;
		case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK:
			return false;
		case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK_END:
			break;
		case Client::Player::PLAYER_UPPER_BODY_STATE::END:
			break;
		default:
			break;
		}
	}
		break;
	case Client::Player::PLAYER_STATE::ROLL_END:
		break;
	case Client::Player::PLAYER_STATE::DIE:
	{
		return false;
	}
		break;
	}

	switch (m_eUpperState)
	{
	case Client::Player::PLAYER_UPPER_BODY_STATE::IDLE:
		break;
	case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK:
		switch (eState)
		{
		case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK_END:
			break;
		default:
			return false;
		}
		break;
	}

	m_eUpperState = eState;

	switch (eState)
	{
	case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK:
		SetAdditiveSequence("CharacterArmature|Sword_Slash");
		break;
	}

	return true;
}

void Client::Player::UpdateState(float fDeltaTime)
{
	switch (m_eState)
	{
	case Client::Player::PLAYER_STATE::IDLE:
		break;
	case Client::Player::PLAYER_STATE::RUN:
		break;
	case Client::Player::PLAYER_STATE::ROLL:
		if (GetAnimation()->GetCurrentSequence()->GetTag() != "CharacterArmature|Roll")
		{
			SetState(PLAYER_STATE::ROLL_END);
		}
		else
		{
			const Engine::Vector3& vPlayerPos = GetTransform()->GetPosition();

			float fHeight = m_pTerrain->GetTerrainHeight(vPlayerPos);

			float fNextHeight = m_pTerrain->GetTerrainHeight(GetTransform()->GetPosition() + m_vRollDir);

			if (vPlayerPos.y >= fNextHeight || fHeight >= fNextHeight - tanf(PI / 4.f))
			{
				GetTransform()->AddPosition(m_vRollDir * m_fRollSpeed * fDeltaTime);
			}
			else
			{
				SetState(PLAYER_STATE::ROLL_END);
			}
		}
		break;
	case Client::Player::PLAYER_STATE::HIT:
	{
		if (GetAnimation()->GetCurrentSequence()->GetTag() != "CharacterArmature|HitRecieve")
		{
			SetState(PLAYER_STATE::HIT_END);
		}
	}
		break;
	case Client::Player::PLAYER_STATE::DIE:
		break;
	case Client::Player::PLAYER_STATE::END:
		break;
	default:
		break;
	}

	switch (m_eUpperState)
	{
	case Client::Player::PLAYER_UPPER_BODY_STATE::IDLE:
		break;
	case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK:
	{
		std::shared_ptr<Engine::Sequence> pAdditiveSequence = GetAnimation()->GetAdditiveSequence();

		if (!pAdditiveSequence || pAdditiveSequence->GetTag() != "CharacterArmature|Sword_Slash")
		{
			SetUpperBodyState(PLAYER_UPPER_BODY_STATE::ATTACK_END);
		}
	}
		break;
	case Client::Player::PLAYER_UPPER_BODY_STATE::ATTACK_END:
		break;
	case Client::Player::PLAYER_UPPER_BODY_STATE::END:
		break;
	default:
		break;
	}
}

void Client::Player::RollEffect(int iFrame, float fTime, Engine::Bindable* pBindable)
{
	std::string strDrawable = "effect";

	strDrawable += std::to_string(iFrame);

	std::shared_ptr<Engine::Drawable> _pDrawable = GetScene()->CreateDrawable<Engine::Drawable>(strDrawable, GetScene()->FindLayer(DEFAULT_LAYER));

	std::shared_ptr<Engine::Animation> pAnimation = std::static_pointer_cast<Engine::Animation>(GetAnimation()->Clone());

	_pDrawable->AddChild(pAnimation);

	pAnimation->SetRate(0.f);

	std::shared_ptr<Engine::Transform> pPlayerTransform = GetTransform();

	std::shared_ptr<Engine::Transform> pTransform = _pDrawable->GetTransform();

	pTransform->SetPosition(pPlayerTransform->GetPosition());

	pTransform->SetScale(pPlayerTransform->GetScale());

	pTransform->SetRotation(pPlayerTransform->GetRotation());

	_pDrawable->FindAndAddBind<Engine::Topology>("TriangleList");
	_pDrawable->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
	_pDrawable->FindAndAddBind<Engine::PixelShader>("AlphaNoUVPS");
	_pDrawable->FindAndAddBind<Engine::InputLayout>("Standard");

	std::shared_ptr<Engine::Material> pMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

	pMaterial = std::static_pointer_cast<Engine::Material>(pMaterial->Clone());

	pMaterial->SetReflectivity(1.f);
	pMaterial->SetDiffuseColor(0.f, 0.f, 1.f, 0.4f);
	pMaterial->SetEmissiveColor({ 0.f, 0.f, 0.f, 0.f });

	std::shared_ptr<Engine::Mesh> pMesh = std::static_pointer_cast<Engine::Mesh>(GetMesh()->Clone());

	int iContainerCount = pMesh->GetMeshCount();

	for (int i = 0; i < iContainerCount; ++i)
	{
		int iSubCount = pMesh->GetMeshSubCount(i);

		for (int j = 0; j < iSubCount; ++j)
		{
			pMesh->SetMaterial(i, j, pMaterial);
		}
	}

	_pDrawable->AddChild(pMesh);
	_pDrawable->NotUseShadow();

	_pDrawable->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);

	m_ShadowList.push_back(_pDrawable);
}

void Client::Player::ChangeSequence(const std::string& strSeq)
{
	GetAnimation()->ChangeSequence(strSeq.c_str());
}

void Client::Player::SetRate(float fRate)
{
	GetAnimation()->SetRate(fRate);
}

void Client::Player::SetAdditiveSequence(const std::string& strSeq)
{
	GetAnimation()->SetAdditiveSequence(strSeq.c_str());
}

void Client::Player::CollisionTerrainStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
	Engine::Collider* pTerrainCollider = pSrc->GetColliderType() == Engine::COLLIDER_TYPE::TERRAIN ? pSrc : pDest;

	const Engine::Vector3& vCross = pTerrainCollider->GetCross();

	Engine::Terrain* pTerrain = static_cast<Engine::Terrain*>(pTerrainCollider->GetParent());

	if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
	{
		pTerrain->AddTerrainHeight(vCross);
	}
	
	else if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::RIGHT))
	{
		pTerrain->SetTileType(vCross, 1);
	}
}

void Client::Player::CollisionPlayerBodyStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
	if (pDest->GetTag() == "frogclawbody")
	{
		Attackable* pAttacker = static_cast<Attackable*>(pDest->GetParent());

		if (pAttacker->Attack(this))
		{
			SetState(Client::Player::PLAYER_STATE::DIE);
		}
		else
		{
			SetState(Client::Player::PLAYER_STATE::HIT);
		}
	}
}

void Client::Player::CollisionCameraLine(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
{
	if (pDest->GetColliderType() == Engine::COLLIDER_TYPE::TERRAIN)
	{
		float fDist = (pDest->GetCross() - static_cast<Engine::ColliderLine*>(pSrc)->GetInfo().vStart).Length();

		Engine::Vector3 vNewPos = GetTransform()->GetPosition();

		vNewPos.y += 1.2f;

		float fPlayer_Cam_Dist = (m_pCamera->GetTransform()->GetPosition() - vNewPos).Length();

		if (fDist < fPlayer_Cam_Dist)
		{
			m_pCamera->GetTransform()->SetPosition(pDest->GetCross());

			m_pCamera->Update(0.f);
		}
	}
}

void Client::Player::CollisionCameraLineLast(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
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

	GetTransform()->SetPosition(10.f, 5.f, 10.f);

	std::shared_ptr<Engine::Mesh> pMesh = CreateBindable<Engine::Mesh>("PlayerMesh", "Medieval.mesh");

	pMesh->UsePaperBurn();

	FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
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

	pAnimation->SetLoop("CharacterArmature|Idle");
	pAnimation->SetLoop("CharacterArmature|Run");
	pAnimation->SetLoop("CharacterArmature|Run_Back");
	pAnimation->SetLoop("CharacterArmature|Run_Left");
	pAnimation->SetLoop("CharacterArmature|Run_Right");
	pAnimation->SetLoop("CharacterArmature|Run_Shoot");
	pAnimation->SetLoop("CharacterArmature|Idle_Gun");
	pAnimation->SetLoop("CharacterArmature|Idle_Gun_Pointing");
	pAnimation->SetLoop("CharacterArmature|Idle_Gun_Shoot");
	pAnimation->SetLoop("CharacterArmature|Idle_Neutral");
	pAnimation->SetLoop("CharacterArmature|Idle_Sword");

	pAnimation->SetNextSequence("CharacterArmature|Roll", "CharacterArmature|Run");
	pAnimation->SetNextSequence("CharacterArmature|Sword_Slash", "CharacterArmature|Idle");
	pAnimation->SetNextSequence("CharacterArmature|HitRecieve", "CharacterArmature|Idle");

	FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
	FindAndAddBind<Engine::DepthStencilState>("OutLineMask");

	m_pBody = CreateBindable<Engine::ColliderOBB>("PlayerBody");

	m_pBody->SetScaleOffset({0.5f, 1.8f, 0.4f});

	m_pBody->SetAxisOffset({0.f, 0.9f, -0.1f});

	m_pBody->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Player::CollisionPlayerBodyStay);

	m_pCamera = Engine::Graphics::GetInst()->GetCamera();

	float fHeight = 2.f;

	float fAngle = PI / 12.f;

	m_pCamera->GetTransform()->SetRelativePosition(0.f, 1.2f + fHeight, fHeight / tanf(fAngle));

	m_pCamera->GetTransform()->SetRelativeRotation(fAngle, PI, 0.f);

	m_pCameraLine = m_pCamera->CreateBindable<Engine::ColliderLine>("cameraline");

	m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &Player::CollisionCameraLine);
	m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::LAST, this, &Player::CollisionCameraLineLast);

	GetTransform()->SetY(10.f);

	for (int i = 0; i < 45; ++i)
	{
		std::string strNotify = "effect";

		strNotify += std::to_string(i + 1);

		std::shared_ptr<Engine::Notify> pNotify = pAnimation->AddNotify("CharacterArmature|Roll", strNotify, 0.048f * i + 0.075f);

		pNotify->SetCallBack(this, &Player::RollEffect);
	}

	m_pSword = GetScene()->CreateDrawable<Attackable>("sword", GetScene()->FindLayer(DEFAULT_LAYER), 50, 20, 25);

	m_pSword->Load(TEXT("UltimateRPGItemsBundle\\Sword\\Sword.fbx"), MESH_PATH);

	std::shared_ptr<Engine::Mesh> pSwordMesh = std::static_pointer_cast<Engine::Mesh>(m_pSword->FindChild(Engine::BINDABLE_TYPE::MESH));

	pSwordMesh->UsePaperBurn();

	m_pSword->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
	m_pSword->FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
	m_pSword->FindAndAddBind<Engine::Topology>("TriangleList");
	m_pSword->FindAndAddBind<Engine::InputLayout>("Standard");
	m_pSword->FindAndAddBind<Engine::DepthStencilState>("OutLineMask");
	m_pSwordBody = m_pSword->CreateBindable<Engine::ColliderOBB>("sword_body");

	m_pSwordBody->Disable();

	m_pSwordBody->SetScaleOffset({ 0.175f, 1.1f, 0.175f });
	m_pSwordBody->SetAxisOffset({ 0.f, 0.4f, 0.f });

	std::shared_ptr<Engine::Material> pSrcMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

	m_pSword->AddChild(pSrcMaterial->Clone());

	m_pSwordParticle = m_pSword->CreateBindable<Engine::Particle>("sword particle", 4096);

	std::shared_ptr<Engine::Transform> pSwordParticleTransform = m_pSwordParticle->GetTransform();

	if (pSwordParticleTransform)
	{
		pSwordParticleTransform->SetRelativePosition(0.f, 1.f, 0.f);
	}
	m_pSwordParticle->SetStartSize({ 0.04f, 0.04f });
	m_pSwordParticle->SetEndSize({ 0.04f, 0.04f });
	m_pSwordParticle->SetMaxCreatePosition({ 0.f, 0.f, 0.f });
	m_pSwordParticle->SetMinCreatePosition({ 0.f, 0.f, 0.f });
	m_pSwordParticle->SetStartColor(Engine::White);
	m_pSwordParticle->SetEndColor({1.f,0.f, 0.f, 0.f});
	m_pSwordParticle->SetMaxParticleCount(4096);
	m_pSwordParticle->SetAccelaration({0.f, -1.f, 0.f});
	m_pSwordParticle->SetVelocity({ -1.f, -1.f, -1.f });
	m_pSwordParticle->SetMaxVelocity({ 1.f, 1.f, 1.f });
	m_pSwordParticle->SetEmitTime(0.001f);
	m_pSwordParticle->SetMaxLifeTime(2.f);
	m_pSwordParticle->SetRenderLayer(Engine::RENDER_LAYER::BLUR);
	m_pSwordParticle->CreateBindable<Engine::Texture>("particletexture", "Particle\\particle_00.png", TEXTURE_PATH);
	m_pSwordParticle->StopEmit();

	m_pJointSocket = pAnimation->AddSocket(40, m_pSword);

	m_pJointSocket->SetRotation({ 0.f, -PI / 2.f, PI / 2.f });
	m_pJointSocket->SetScale(0.5f, 0.5f, 0.5f);
	m_pJointSocket->SetPosition({ 0.f, -0.03f, 0.1f });

	m_pTrail = GetScene()->CreateDrawable<Trail>("Trail", GetScene()->FindLayer(DEFAULT_LAYER), 10);

	for (int i = 0; i < 77; ++i)
	{
		std::string strNotify = "trail";

		strNotify += std::to_string(i);

		std::shared_ptr<Engine::Notify> pNotify = pAnimation->AddNotify("CharacterArmature|Sword_Slash", strNotify, i * 0.01666f);

		if (i == 0)
		{
			pNotify->SetCallBack(
				[this](int iFrame, float fTime, Engine::Bindable* pOwner)
				{
					Engine::Vector3 vTop = { 0.f, 2.0f, 0.f };
					Engine::Vector3 vBottom = { 0.f, 0.3f, 0.f };

					const Engine::Matrix& matTransform = m_pSword->GetTransform()->GetTransformMatrix();

					m_pTrail->SetAllPosition(matTransform.TransformCoord(vTop), matTransform.TransformCoord(vBottom));
					m_pTrail->Enable();
					m_pSwordBody->Enable();
					m_pSwordParticle->ResumeEmit();
				});
		}
		else if (i == 76)
		{
			pNotify->SetCallBack(
				[this](int, float, Engine::Bindable*)
				{
					m_pTrail->Disable();
					m_pSwordBody->Disable();
					m_pSwordParticle->StopEmit();
				});
		}
		else
		{
			pNotify->SetCallBack(
				[this](int iFrame, float fTime, Engine::Bindable* pOwner)
				{
					Engine::Vector3 vTop = { 0.f, 2.0f, 0.f };
					Engine::Vector3 vBottom = { 0.f, 0.3f, 0.f };

					const Engine::Matrix& matTransform = m_pSword->GetTransform()->GetTransformMatrix();

					m_pTrail->SetPosition(matTransform.TransformCoord(vTop), matTransform.TransformCoord(vBottom));
				});

		}
	}

	std::shared_ptr<Engine::Notify> pDieNotify = pAnimation->AddNotify("CharacterArmature|Death", "DiePaperBurn", 1.f);

	pDieNotify->SetCallBack([this](int, float, Engine::Bindable*) 
		{
			StartPaperBurn();
		}
	);

	Engine::CInput::GetInst()->AddKey(DIK_LCONTROL);

	return true;
}

void Client::Player::Input(float fDeltaTime)
{
	std::shared_ptr<Engine::Transform> pTransform = GetTransform();

	std::shared_ptr<Engine::Transform> pCamTransform = m_pCamera->GetTransform();

	Engine::Vector3  vDir = {};

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_W))
	{
		vDir = -pTransform->GetAxis(Engine::AXIS_TYPE::Z);
		m_eDir = MOVE_DIR::UP;
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_S))
	{
		vDir = pTransform->GetAxis(Engine::AXIS_TYPE::Z);
		m_eDir = MOVE_DIR::DOWN;
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_D))
	{
		vDir = -pTransform->GetAxis(Engine::AXIS_TYPE::X);
		m_eDir = MOVE_DIR::RIGHT;
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_A))
	{
		vDir = pTransform->GetAxis(Engine::AXIS_TYPE::X);
		m_eDir = MOVE_DIR::LEFT;
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_SPACE))
	{
		m_vRollDir = -pTransform->GetAxis(Engine::AXIS_TYPE::Z);
		SetState(PLAYER_STATE::ROLL);
	}

	if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::UP, DIK_LCONTROL))
	{
		if (m_bCanJump)
		{
			m_bCanJump = false;

			m_fFallSpeed += 4.5f;

			pTransform->AddY(m_fFallSpeed * fDeltaTime);
		}
	}

	if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
	{
		SetUpperBodyState(PLAYER_UPPER_BODY_STATE::ATTACK);
	}

	if (vDir != 0.f)
	{
		const Engine::Vector3& vPlayerPos = pTransform->GetPosition();

		float fHeight = m_pTerrain->GetTerrainHeight(vPlayerPos);

		float fNextHeight = m_pTerrain->GetTerrainHeight(vPlayerPos + vDir);

		vDir.y = fNextHeight - fHeight;

		vDir.Normalize();

		if (fNextHeight < vPlayerPos.y || fHeight >= fNextHeight - tanf(PI / 4.f))
		{
			if (SetState(PLAYER_STATE::RUN))
			{
				if (fNextHeight < vPlayerPos.y)
				{
					vDir.y = 0.f;

					vDir.Normalize();

					pTransform->AddPosition(vDir * fDeltaTime * m_fSpeed);
				}
				else
				{
					pTransform->AddPosition(vDir * fDeltaTime * m_fSpeed);
				}

				pTransform->SetRY(pCamTransform->GetRY() + PI);
			}
		}
		else
		{
			SetState(PLAYER_STATE::IDLE);
		}
	}
	else
	{
		SetState(PLAYER_STATE::IDLE);
	}

	if (!Engine::Window::GetInst()->IsLockRotation())
	{
		int iDeltaX = Engine::CInput::GetInst()->GetMouseDeltaX();
		int iDeltaY = Engine::CInput::GetInst()->GetMouseDeltaY();

		if (iDeltaX)
		{
			pCamTransform->AddRelativeRY(iDeltaX * fDeltaTime);
		}

		if (iDeltaY && pTransform->GetRX() + iDeltaY * fDeltaTime <= PI && pTransform->GetRX() + iDeltaY * fDeltaTime >= -PI)
		{
			pCamTransform->AddRelativeRX(iDeltaY * fDeltaTime);
		}

		if (iDeltaX || iDeltaY)
		{
			pCamTransform->PostUpdate(0.f);
		}

		m_fCameraDist += Engine::CInput::GetInst()->GetMouseDeltaZ() / 120 * 5.f;
	}

	Engine::Vector3 vNewPos = pTransform->GetPosition() - pCamTransform->GetAxis(Engine::AXIS_TYPE::Z) * m_fCameraDist;

	vNewPos.y += 1.2f;

	pCamTransform->SetPosition(vNewPos);

	m_pCameraLine->SetEndOffset(pCamTransform->GetAxis(Engine::AXIS_TYPE::Z));
}

void Client::Player::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (!m_pTerrain)
	{
		return;
	}

	std::shared_ptr<Engine::Transform> pTransform = GetTransform();

	pTransform->AddY(m_fFallSpeed * fDeltaTime);

	m_fFallSpeed += m_fAccel * fDeltaTime;

	float fHeight = m_pTerrain->GetTerrainHeight(pTransform->GetPosition());

	if (fHeight > pTransform->GetY())
	{
		pTransform->SetY(fHeight);

		m_fFallSpeed /= 2.f;

		m_bCanJump = true;
	}

	UpdateState(fDeltaTime);
}

void Client::Player::FixedUpdate(float fDeltaTime)
{
	__super::FixedUpdate(fDeltaTime);

	std::list<SHADOWINFO>::iterator iter = m_ShadowList.begin();
	std::list<SHADOWINFO>::iterator iterEnd = m_ShadowList.end();

	for (; iter != iterEnd;)
	{
		if (++(*iter).iFrame >= m_iMaxShadowFrame)
		{
			(*iter).pDrawable->InActivate();
			iter = m_ShadowList.erase(iter);
			iterEnd = m_ShadowList.end();
			continue;
		}
		std::shared_ptr<Engine::Mesh> pMesh = std::static_pointer_cast<Engine::Mesh>((*iter).pDrawable->FindChild(Engine::BINDABLE_TYPE::MESH));

		if (!pMesh)
		{
			continue;
		}

		std::shared_ptr<Engine::Material> pMaterial = pMesh->GetMaterial();

		if (!pMaterial)
		{
			continue;
		}

		float fRate = (*iter).iFrame / static_cast<float>(m_iMaxShadowFrame);

		Engine::Vector4 vEndColor = { 65.f / 255.f, 92.f / 255.f, 250.f / 255.f, 0.0f };
		Engine::Vector4 vStartColor = { 199.f / 255.f, 20.f / 255.f, 231.f / 255.f, 0.5f };

		pMaterial->SetDiffuseColor(vEndColor * fRate + (1.f - fRate) * vStartColor);


		++iter;
	}
}

void Client::Player::PostUpdate(float fDeltaTime)
{
	__super::PostUpdate(fDeltaTime);
}

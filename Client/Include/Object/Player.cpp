#include "Player.h"
#include "Core/Graphics.h"
#include "Bindable/Camera.h"
#include "Input/Input.h"
#include "Bindable/Transform.h"
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
#include "Bindable/UIRenderer.h"
#include "../UI/Inventory.h"
#include "Bindable/SoundBindable.h"
#include "Bullet.h"

namespace Client
{
	Player::Player(int iMaxHP, int iAttackMin, int iAttackMax) :
		Attackable(iMaxHP, iAttackMin, iAttackMax)
		, m_fSpeed(5.f)
		, m_fAccel(-9.8f)
		, m_fFallSpeed(0.f)
		, m_fRollSpeed(7.f)
		, m_eState(PLAYER_STATE::IDLE)
		, m_eUpperState(PLAYER_UPPER_BODY_STATE::IDLE)
		, m_iMaxShadowFrame(15)
		, m_fCameraDist(2.f)
		, m_bCanJump(true)
	{
	}

	Player::~Player()
	{
		if (m_pCameraLine)
		{
			m_pCameraLine->ClearCallBack();
		}
	}

	bool Player::SetState(PLAYER_STATE eState)
	{
		switch (m_eState)
		{
		case PLAYER_STATE::IDLE:
			break;
		case PLAYER_STATE::RUN:
			switch (eState)
			{
			case PLAYER_STATE::IDLE:
				break;
			case PLAYER_STATE::RUN:
				break;
			case PLAYER_STATE::ROLL:
				break;
			case PLAYER_STATE::DIE:
				break;
			case PLAYER_STATE::END:
				break;
			default:
				break;
			}
			break;
		case PLAYER_STATE::ROLL:
			switch (eState)
			{
			case PLAYER_STATE::ROLL_END:
			case PLAYER_STATE::DIE:
				m_pBody->Enable();
				break;
			default:
				return false;
			}
			break;
		case PLAYER_STATE::HIT:
		{
			switch (eState)
			{
			case PLAYER_STATE::HIT_END:
			case PLAYER_STATE::DIE:
				break;
			default:
				return false;
			}
		}
		break;
		case PLAYER_STATE::HIT_END:
			break;
		case PLAYER_STATE::DIE:
			return false;
		}

		m_eState = eState;

		switch (eState)
		{
		case PLAYER_STATE::IDLE:
			ChangeSequence("CharacterArmature|Idle");
			break;
		case PLAYER_STATE::RUN:

			switch (m_eDir)
			{
			case Player::MOVE_DIR::LEFT:
				ChangeSequence("CharacterArmature|Run_Left");
				break;
			case Player::MOVE_DIR::RIGHT:
				ChangeSequence("CharacterArmature|Run_Right");
				break;
			case Player::MOVE_DIR::UP:
				ChangeSequence("CharacterArmature|Run");
				break;
			case Player::MOVE_DIR::DOWN:
				ChangeSequence("CharacterArmature|Run_Back");
				break;
			case Player::MOVE_DIR::END:
				break;
			default:
				break;
			}
			break;
		case PLAYER_STATE::ROLL:
			m_pBody->Disable();
			SetRate(2.f);
			ChangeSequence("CharacterArmature|Roll");
			break;
		case PLAYER_STATE::ROLL_END:
			SetRate(1.f);
			break;
		case PLAYER_STATE::HIT:
			ChangeSequence("CharacterArmature|HitRecieve");
			break;
		case PLAYER_STATE::DIE:
			ChangeSequence("CharacterArmature|Death");
			break;
		}

		return true;
	}

	bool Player::SetUpperBodyState(PLAYER_UPPER_BODY_STATE eState)
	{
		switch (m_eState)
		{
		case PLAYER_STATE::IDLE:
			break;
		case PLAYER_STATE::RUN:
			break;
		case PLAYER_STATE::ROLL:
		{
			switch (eState)
			{
			case PLAYER_UPPER_BODY_STATE::IDLE:
				break;
			case PLAYER_UPPER_BODY_STATE::ATTACK:
				return false;
			case PLAYER_UPPER_BODY_STATE::ATTACK_END:
				break;
			case PLAYER_UPPER_BODY_STATE::END:
				break;
			default:
				break;
			}
		}
		break;
		case PLAYER_STATE::ROLL_END:
			break;
		case PLAYER_STATE::DIE:
		{
			return false;
		}
		break;
		}

		switch (m_eUpperState)
		{
		case PLAYER_UPPER_BODY_STATE::IDLE:
			break;
		case PLAYER_UPPER_BODY_STATE::ATTACK:
			switch (eState)
			{
			case PLAYER_UPPER_BODY_STATE::ATTACK_END:
				break;
			default:
				return false;
			}
			break;
		}

		m_eUpperState = eState;

		switch (eState)
		{
		case PLAYER_UPPER_BODY_STATE::IDLE:
			
			break;
		case PLAYER_UPPER_BODY_STATE::ATTACK:
		{
			if (m_pInventory)
			{
				WEAPON_TYPE eWeaponType = m_pInventory->GetEquipWeaponType(Inventory::EQUIP_SLOT::HAND_RIGHT);

				switch (eWeaponType)
				{
				case WEAPON_TYPE::FIST:
					SetAdditiveSequence("CharacterArmature|Punch_Left");
					break;
				case WEAPON_TYPE::SWORD:
					SetAdditiveSequence("CharacterArmature|Sword_Slash");
					break;
				case WEAPON_TYPE::GUN:
				{
					SetAdditiveSequence("CharacterArmature|Idle_Gun_Shoot");

					std::shared_ptr<Bullet> pBullet = GetScene()->CreateDrawable<Bullet>("bullet", GetScene()->FindLayer(DEFAULT_LAYER));

					std::shared_ptr<Engine::Transform> pBulletTransform = pBullet->GetTransform();

					if (m_pSword)
					{
						pBulletTransform->SetPosition(m_pSword->GetTransform()->GetPosition());

						pBulletTransform->SetAxis(Engine::AXIS_TYPE::Y, m_pSword->GetTransform()->GetAxis(Engine::AXIS_TYPE::X));
					}
				}
				break;
				case WEAPON_TYPE::END:
					break;
				default:
					break;
				}
			}

		}
			break;
		}

		return true;
	}

	void Player::UpdateState(float fDeltaTime)
	{
		switch (m_eState)
		{
		case PLAYER_STATE::IDLE:
			break;
		case PLAYER_STATE::RUN:
			break;
		case PLAYER_STATE::ROLL:
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
		case PLAYER_STATE::HIT:
		{
			if (GetAnimation()->GetCurrentSequence()->GetTag() != "CharacterArmature|HitRecieve")
			{
				SetState(PLAYER_STATE::HIT_END);
			}
		}
		break;
		case PLAYER_STATE::DIE:
			break;
		case PLAYER_STATE::END:
			break;
		default:
			break;
		}

		switch (m_eUpperState)
		{
		case PLAYER_UPPER_BODY_STATE::IDLE:
			break;
		case PLAYER_UPPER_BODY_STATE::ATTACK:
		{
			std::shared_ptr<Engine::Sequence> pAdditiveSequence = GetAnimation()->GetAdditiveSequence();

			if (!pAdditiveSequence || pAdditiveSequence->GetTag() != "CharacterArmature|Sword_Slash")
			{
				SetUpperBodyState(PLAYER_UPPER_BODY_STATE::ATTACK_END);
			}
		}
		break;
		case PLAYER_UPPER_BODY_STATE::ATTACK_END:
			break;
		case PLAYER_UPPER_BODY_STATE::END:
			break;
		default:
			break;
		}
	}

	void Player::RollEffect(int iFrame, float fTime, Engine::Bindable* pBindable)
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
		_pDrawable->FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoSpecMapNoNormalMap");
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

	void Player::ChangeSequence(const std::string& strSeq)
	{
		GetAnimation()->ChangeSequence(strSeq.c_str());
	}

	void Player::SetRate(float fRate)
	{
		GetAnimation()->SetRate(fRate);
	}

	void Player::SetAdditiveSequence(const std::string& strSeq)
	{
		GetAnimation()->SetAdditiveSequence(strSeq.c_str());
	}

	void Player::ChangeWeaponMesh(std::shared_ptr<Engine::Drawable> pDrawable)
	{
		if (m_pJointSocket)
		{
			GetAnimation()->DeleteSocket(m_pJointSocket);
		}

		if (m_pSword)
		{
			m_pSword->Disable();
		}

		m_pSword = pDrawable;

		if (pDrawable)
		{
			// Phase B.4 — SoundBindable migrated to Component; look up
			// via FindComponent rather than the old Bindable child-list.
			m_pSwordSound = std::static_pointer_cast<Engine::SoundBindable>(pDrawable->FindComponent("weapon sound"));

			// Phase B.4 — Collider migrated to Component; FindComponent.
			m_pSwordBody = std::static_pointer_cast<Engine::ColliderOBB>(pDrawable->FindComponent(Engine::COMPONENT_TYPE::COLLIDER_OBB));

			m_pSwordParticle = std::static_pointer_cast<Engine::Particle>(pDrawable->FindChild(pDrawable->GetTag() + " particle"));

			m_pJointSocket = GetAnimation()->AddSocket(40, m_pSword);

			m_pJointSocket->SetRotation({ 0.f, -PI / 2.f, PI / 2.f });
			m_pJointSocket->SetScale(0.5f, 0.5f, 0.5f);
			m_pJointSocket->SetPosition({ 0.f, -0.03f, 0.1f });
		}
		else
		{
			m_pSwordBody = nullptr;

			m_pSwordParticle = nullptr;

			m_pSwordSound = nullptr;
		}
	}

	void Player::ChangeArmorMesh(std::shared_ptr<Engine::Drawable> pDrawable)
	{
		if (m_pJointSocketArmor)
		{
			GetAnimation()->DeleteSocket(m_pJointSocketArmor);
		}

		if (pDrawable)
		{
			m_pJointSocketArmor = GetAnimation()->AddSocket(5, pDrawable);

			m_pJointSocketArmor->SetRotation({ PI / 2.f, 0.f, 0.f });
			m_pJointSocketArmor->SetScale(0.4f, 0.4f, 0.4f);
			m_pJointSocketArmor->SetPosition({ 0.f, 0.03f, 0.f });
		}
	}

	std::shared_ptr<Engine::Drawable> Player::GetWeapon() const
	{
		return m_pSword;
	}

	void Player::SetInventory(std::shared_ptr<Inventory> pInventory)
	{
		m_pInventory = pInventory;
	}

	void Player::CollisionTerrainStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (!m_pInventory)
		{
			return;
		}

		int iItemID = m_pInventory->GetEquipItem(Inventory::EQUIP_SLOT::HAND_RIGHT);

		if (pDest->GetTag() != "MouseLine")
		{
			return;
		}

		Engine::Collider* pTerrainCollider = pSrc;

		const Engine::Vector3& vCross = pTerrainCollider->GetCross();

		// Phase B.4 — Collider migrated to Component; owner via GetOwner.
		Engine::Terrain* pTerrain = static_cast<Engine::Terrain*>(pTerrainCollider->GetOwner());

		if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
		{
			switch (iItemID)
			{
			case 2:
				pTerrain->AddTerrainHeight(vCross);
				break;
			case 3:
			{
				int iType = pTerrain->GetTileType(vCross);

				pTerrain->SetTileType(vCross, (iType + 1) % 7);
			}
				break;
			default:
				break;
			}
		}

		else if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::RIGHT))
		{
			switch (iItemID)
			{
			case 2:
				pTerrain->AddTerrainHeight(vCross, -1);
				break;
			case 3:
			{
				int iType = pTerrain->GetTileType(vCross);

				pTerrain->SetTileType(vCross, (7 + iType - 1) % 7);
			}
				break;
			default:
				break;
			}
		}

		else if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::WHEEL))
		{
			pTerrain->SetTileType(vCross, 1);
		}
	}

	void Player::CollisionPlayerBodyStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (pDest->GetTag() == "frogclawbody")
		{
			// Phase B.4 — Collider migrated to Component; owner via GetOwner.
			Attackable* pAttacker = static_cast<Attackable*>(pDest->GetOwner());

			if (pAttacker->Attack(this))
			{
				SetState(PLAYER_STATE::DIE);
			}
			else
			{
				SetState(PLAYER_STATE::HIT);
			}
		}
	}

	void Player::CollisionCameraLine(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
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

	void Player::CollisionCameraLineLast(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
	}

	void Player::Start()
	{
		__super::Start();

		std::shared_ptr<Engine::Layer> pLayer = GetScene()->FindLayer(DEFAULT_LAYER);

		m_pTerrain = std::static_pointer_cast<Engine::Terrain>(pLayer->FindDrawable(Engine::BINDABLE_TYPE::TERRAIN));
	}

	bool Player::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		GetTransform()->SetPosition(10.f, 5.f, 10.f);
		//GetTransform()->SetScale(0.01f, 0.01f, 0.01f);

		std::shared_ptr<Engine::Mesh> pMesh = CreateBindable<Engine::Mesh>("PlayerMesh", "Walking.mesh");

		pMesh->UsePaperBurn();

		FindAndAddBind<Engine::VertexShader>(STANDARD_ANIM_VS);
		FindAndAddBind<Engine::InputLayout>("Standard");
		FindAndAddBind<Engine::Topology>("TriangleList");

		FindAndAddBind<Engine::PixelShader>(STANDARD_PS);
		FindAndAddBind<Engine::DepthStencilState>("OutLineMask");

		std::shared_ptr<Engine::Animation> pAnimation = CreateBindable<Engine::Animation>("PlayerAnimation");

		std::vector<std::string> vecSeq = {
			"mixamo.com",
			/*"CharacterArmature|Gun_Shoot",
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
			"CharacterArmature|Wave",*/
		};

		for (size_t i = 0; i < vecSeq.size(); ++i)
		{
			if (std::shared_ptr<Engine::Sequence> pSequence = Engine::ResourceManager::GetInst()->FindSequence(vecSeq[i]))
			{
				pSequence->UseRootMotion();

				pAnimation->AddSequance(pSequence->GetTag(), pSequence);
			}
		}

		std::shared_ptr<Engine::Skeleton> pSkeleton = Engine::ResourceManager::GetInst()->FindSkeleton("Walking");

		assert(pSkeleton);

		pAnimation->SetSkeleton(pSkeleton);

		//pAnimation->ChangeSequence("CharacterArmature|Idle");

		pAnimation->SetLoop("mixamo.com");
		/*pAnimation->SetLoop("CharacterArmature|Run");
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
		pAnimation->SetNextSequence("CharacterArmature|HitRecieve", "CharacterArmature|Idle");*/

		m_pBody = CreateComponent<Engine::ColliderOBB>("PlayerBody");

		m_pBody->SetScaleOffset({ 0.5f, 1.8f, 0.4f });

		m_pBody->SetAxisOffset({ 0.f, 0.9f, -0.1f });

		m_pBody->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Player::CollisionPlayerBodyStay);

		m_pCamera = Engine::Graphics::GetInst()->GetCamera();

		float fHeight = 2.f;

		float fAngle = PI / 12.f;

		m_pCamera->GetTransform()->SetRelativePosition(0.f, 1.2f + fHeight, fHeight / tanf(fAngle));

		m_pCamera->GetTransform()->SetRelativeRotation(fAngle, PI, 0.f);

		m_pCameraLine = m_pCamera->CreateComponent<Engine::ColliderLine>("cameraline");

		m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &Player::CollisionCameraLine);
		m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::LAST, this, &Player::CollisionCameraLineLast);

		m_pTrail = GetScene()->CreateDrawable<Trail>("Trail", GetScene()->FindLayer(DEFAULT_LAYER), 10);

		/*for (int i = 0; i < 77; ++i)
		{
			std::string strNotify = "trail";

			strNotify += std::to_string(i);

			std::shared_ptr<Engine::Notify> pNotify = GetAnimation()->AddNotify("CharacterArmature|Sword_Slash", strNotify, i * 0.01666f);

			if (i == 0)
			{
				pNotify->SetCallBack(
					[this](int iFrame, float fTime, Engine::Bindable* pOwner)
					{
						if (!m_pSword)
						{
							return;
						}

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
						if (!m_pSword)
						{
							return;
						}

						m_pTrail->Disable();
						m_pSwordBody->Disable();
						m_pSwordParticle->StopEmit();
					});
			}
			else if (i == 30)
			{
				pNotify->SetCallBack(
					[this](int, float, Engine::Bindable*)
					{

						if (m_pSwordSound)
						{
							m_pSwordSound->Play();
						}
					});
			}
			else
			{
				pNotify->SetCallBack(
					[this](int iFrame, float fTime, Engine::Bindable* pOwner)
					{
						if (!m_pSword)
						{
							return;
						}

						Engine::Vector3 vTop = { 0.f, 2.0f, 0.f };
						Engine::Vector3 vBottom = { 0.f, 0.3f, 0.f };

						const Engine::Matrix& matTransform = m_pSword->GetTransform()->GetTransformMatrix();

						m_pTrail->SetPosition(matTransform.TransformCoord(vTop), matTransform.TransformCoord(vBottom));
					});

			}
		}
		GetTransform()->SetY(10.f);

		for (int i = 0; i < 45; ++i)
		{
			std::string strNotify = "effect";

			strNotify += std::to_string(i + 1);

			std::shared_ptr<Engine::Notify> pNotify = pAnimation->AddNotify("CharacterArmature|Roll", strNotify, 0.048f * i + 0.075f);

			pNotify->SetCallBack(this, &Player::RollEffect);
		}

		std::shared_ptr<Engine::Notify> pDieNotify = pAnimation->AddNotify("CharacterArmature|Death", "DiePaperBurn", 1.f);

		pDieNotify->SetCallBack([this](int, float, Engine::Bindable*)
			{
				StartPaperBurn();
			}
		);

		Engine::CInput::GetInst()->AddKey(DIK_LCONTROL);

		m_pFootLSound = CreateComponent<Engine::SoundBindable>("step_rock_l", "step_rock_l");

		m_pFootRSound = CreateComponent<Engine::SoundBindable>("step_rock_r", "step_rock_r");

		std::shared_ptr<Engine::Notify> pRunLNotify = pAnimation->AddNotify("CharacterArmature|Run", "foot_l", 0.5f);

		pRunLNotify->SetCallBack([this](int, float, Engine::Bindable*)
			{
				m_pFootLSound->Play();
			}
		);

		std::shared_ptr<Engine::Notify> pRunRNotify = pAnimation->AddNotify("CharacterArmature|Run", "foot_r", 0.9f);

		pRunRNotify->SetCallBack([this](int, float, Engine::Bindable*)
			{
				m_pFootRSound->Play();
			}
		);*/

		return true;
	}

	void Player::Input(float fDeltaTime)
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

			float fHeight = m_pTerrain ? m_pTerrain->GetTerrainHeight(vPlayerPos) : 0.f;

			float fNextHeight = m_pTerrain ? m_pTerrain->GetTerrainHeight(vPlayerPos + vDir) : 0.f;

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

	void Player::Update(float fDeltaTime)
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

	void Player::FixedUpdate(float fDeltaTime)
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

	void Player::PostUpdate(float fDeltaTime)
	{
		__super::PostUpdate(fDeltaTime);
	}

}
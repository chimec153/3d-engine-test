#include "Player.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT_EX(Player, new Client::Player(100, 1, 5))
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
#include "GameObject/GameObject.h"
#include "Bindable/ColliderOBB.h"
#include "Bindable/Particle.h"
#include "Bindable/Texture.h"
#include "Attackable.h"
#include "Bindable/Decal.h"
#include "Bindable/ColliderLine.h"
#include "Bindable/UIRenderer.h"
#include "../UI/Inventory.h"
#include "Bindable/SoundBindable.h"
#include "Bullet.h"
#include "PlayerState.h"
#include "Voxel/VoxelWorld.h"
#include "Scene/Scene.h"
#include "../Scene/GameScene.h"
#include <cmath>

namespace Client
{
	Player::Player(int iMaxHP, int iAttackMin, int iAttackMax) :
		Engine::GameObject()
		, m_iInitHP(iMaxHP)
		, m_iInitAttackMin(iAttackMin)
		, m_iInitAttackMax(iAttackMax)
		, m_fSpeed(5.f)
		, m_fRollSpeed(7.f)
		, m_iMaxShadowFrame(15)
		, m_fCameraDist(2.f)
	{
	}

	Player::~Player()
	{
		if (m_pCameraLine)
		{
			m_pCameraLine->ClearCallBack();
		}
	}

	bool Player::ChangeLowerState(std::unique_ptr<IPlayerLowerState> pNext)
	{
		if (!pNext) return false;

		// Transition gates lifted from the original SetState matrix.
		// - Roll only accepts RollEnd/Die transitions (and re-enables the
		//   body collider that Roll::Enter disabled).
		// - Hit only accepts HitEnd/Die.
		// - Die is absorbing.
		auto* pCurr = m_lowerStateMachine.GetCurrent();
		if (pCurr)
		{
			const bool bIsRoll  = dynamic_cast<PlayerLowerRollState*>(pCurr) != nullptr;
			const bool bIsHit   = dynamic_cast<PlayerLowerHitState*>(pCurr)  != nullptr;
			const bool bIsDie   = dynamic_cast<PlayerLowerDieState*>(pCurr)  != nullptr;

			const bool bToRollEnd = dynamic_cast<PlayerLowerRollEndState*>(pNext.get()) != nullptr;
			const bool bToHitEnd  = dynamic_cast<PlayerLowerHitEndState*>(pNext.get())  != nullptr;
			const bool bToDie     = dynamic_cast<PlayerLowerDieState*>(pNext.get())     != nullptr;

			if (bIsDie) return false;
			if (bIsRoll && !(bToRollEnd || bToDie)) return false;
			if (bIsHit  && !(bToHitEnd  || bToDie)) return false;
		}

		m_lowerStateMachine.ChangeState(std::move(pNext));

		// Anim sync — same role as the trailing switch in original SetState.
		if (m_pAnimation)
		{
			const char* pSeq = m_lowerStateMachine.GetCurrentAnimSequence();
			if (pSeq && pSeq[0])
				m_pAnimation->ChangeSequence(pSeq);
		}
		return true;
	}

	bool Player::ChangeUpperState(std::unique_ptr<IPlayerUpperState> pNext)
	{
		if (!pNext) return false;

		// Roll/Die in the lower machine forbid Attack on the upper machine,
		// mirroring original SetUpperBodyState's outer switch.
		auto* pLower = m_lowerStateMachine.GetCurrent();
		const bool bLowerRoll = dynamic_cast<PlayerLowerRollState*>(pLower) != nullptr;
		const bool bLowerDie  = dynamic_cast<PlayerLowerDieState*>(pLower)  != nullptr;
		const bool bToAttack  = dynamic_cast<PlayerUpperAttackState*>(pNext.get()) != nullptr;

		if (bLowerDie) return false;
		if (bLowerRoll && bToAttack) return false;

		// Upper Attack only releases to AttackEnd.
		auto* pUpper = m_upperStateMachine.GetCurrent();
		const bool bIsAttack  = dynamic_cast<PlayerUpperAttackState*>(pUpper) != nullptr;
		const bool bToAttackEnd = dynamic_cast<PlayerUpperAttackEndState*>(pNext.get()) != nullptr;
		if (bIsAttack && !bToAttackEnd) return false;

		m_upperStateMachine.ChangeState(std::move(pNext));
		return true;
	}

	void Player::UpdateState(float fDeltaTime)
	{
		// Dispatch through Change* so transition gates + anim sync run on
		// every state-driven swap (not just the input-driven ones).
		if (auto* pLower = m_lowerStateMachine.GetCurrent())
		{
			if (auto pNext = pLower->Update(*this, fDeltaTime))
			{
				ChangeLowerState(
					std::unique_ptr<IPlayerLowerState>(static_cast<IPlayerLowerState*>(pNext.release())));
			}
		}
		if (auto* pUpper = m_upperStateMachine.GetCurrent())
		{
			if (auto pNext = pUpper->Update(*this, fDeltaTime))
			{
				ChangeUpperState(
					std::unique_ptr<IPlayerUpperState>(static_cast<IPlayerUpperState*>(pNext.release())));
			}
		}
	}

	void Player::RollEffect(int iFrame, float fTime, Engine::Bindable* pBindable)
	{
		std::string strName = "effect";
		strName += std::to_string(iFrame);

		// Phase E5 — shadow effect entity is a GameObject with cloned mesh
		// + animation, rendered in the alpha pass via MeshRendererComponent.
		std::shared_ptr<Engine::GameObject> pShadowObj =
			GetScene()->CreateGameObject<>(strName, GetScene()->FindLayer(DEFAULT_LAYER));
		if (!pShadowObj) return;

		auto pTransform    = pShadowObj->AddComponent<Engine::Transform>("transform");
		auto pMeshRenderer = pShadowObj->AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
		auto pAnimation    = std::static_pointer_cast<Engine::Animation>(m_pAnimation->Clone());
		if (pAnimation)
		{
			pShadowObj->AddComponent(pAnimation);
			pAnimation->SetRate(0.f);
		}

		if (pTransform && m_pTransform)
		{
			pTransform->SetPosition(m_pTransform->GetPosition());
			pTransform->SetScale(m_pTransform->GetScale());
			pTransform->SetRotation(m_pTransform->GetRotation());
		}

		std::shared_ptr<Engine::Mesh> pMesh = m_pMeshRenderer && m_pMeshRenderer->GetMesh()
			? std::static_pointer_cast<Engine::Mesh>(m_pMeshRenderer->GetMesh()->Clone())
			: nullptr;
		if (!pMesh) return;

		std::shared_ptr<Engine::Material> pMaterial = Engine::StaticFindBindable<Engine::Material>("Material");
		if (pMaterial)
		{
			pMaterial = std::static_pointer_cast<Engine::Material>(pMaterial->Clone());
			pMaterial->SetReflectivity(1.f);
			pMaterial->SetDiffuseColor(0.f, 0.f, 1.f, 0.4f);
			pMaterial->SetEmissiveColor({ 0.f, 0.f, 0.f, 0.f });
		}

		int iContainerCount = pMesh->GetMeshCount();
		for (int i = 0; i < iContainerCount; ++i)
		{
			int iSubCount = pMesh->GetMeshSubCount(i);
			for (int j = 0; j < iSubCount; ++j)
				pMesh->SetMaterial(i, j, pMaterial);
		}

		if (pMeshRenderer)
		{
			pMeshRenderer->SetMesh(pMesh);
			pMeshRenderer->SetMaterial(pMaterial);
			pMeshRenderer->SetAnimation(pAnimation);
			pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
			pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>("anisotropic_microfacet VSSkin"));
			pMeshRenderer->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>("anisotropic_microfacet PS_NoSpecMapNoNormalMap"));
			pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
			pMeshRenderer->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
		}

		m_ShadowList.emplace_back(pShadowObj, pMeshRenderer);
	}

	void Player::ChangeSequence(const std::string& strSeq)
	{
		m_pAnimation->ChangeSequence(strSeq.c_str());
	}

	void Player::SetRate(float fRate)
	{
		m_pAnimation->SetRate(fRate);
	}

	void Player::SetAdditiveSequence(const std::string& strSeq)
	{
		m_pAnimation->SetAdditiveSequence(strSeq.c_str());
	}

	// Phase E5 — ChangeWeaponMesh / ChangeArmorMesh / GetWeapon removed.
	// Their callers all came from Inventory's UpdateEquipSlot, which has
	// been stubbed out (Inventory creation is commented out in GameScene).
	// Reintroduce under a GameObject-based weapon-equip path when the
	// inventory UI is rebuilt.

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

		// Phase E5 — Terrain is now a GameObject (the collider's host).
		Engine::Terrain* pTerrain = static_cast<Engine::Terrain*>(pTerrainCollider->GetGameObjectOwner());
		if (!pTerrain) return;

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
			// Phase E5 — attacker is GameObject-hosted (Drawable hosts gone).
			std::shared_ptr<Attackable> pAttacker;
			if (Engine::GameObject* pOwnerGameObject = pDest->GetGameObjectOwner())
				pAttacker = pOwnerGameObject->GetComponent<Attackable>();

			if (pAttacker && pAttacker->Attack(m_pAttackable.get()))
			{
				ChangeLowerState(std::make_unique<PlayerLowerDieState>());
			}
			else
			{
				ChangeLowerState(std::make_unique<PlayerLowerHitState>());
			}
		}
	}

	void Player::CollisionCameraLine(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (pDest->GetColliderType() == Engine::COLLIDER_TYPE::TERRAIN)
		{
			float fDist = (pDest->GetCross() - static_cast<Engine::ColliderLine*>(pSrc)->GetInfo().vStart).Length();

			Engine::Vector3 vNewPos = m_pTransform->GetPosition();

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

		// Phase E5 — Terrain is a GameObject now; lookup via FindGameObject.
		m_pTerrain = std::static_pointer_cast<Engine::Terrain>(pLayer->FindGameObject("Terrain"));
	}

	bool Player::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		// Phase E5 — assemble entity from Components.
		m_pTransform    = AddComponent<Engine::Transform>("transform");
		m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
		m_pAnimation    = AddComponent<Engine::Animation>("PlayerAnimation");

		// Attackable's Init runs GetOwner()->CreateComponent for its
		// siblings, but Player is a GameObject now (no Drawable owner).
		// Attackable handles a null owner gracefully (PaperBurn/Particle/
		// Sound creation just becomes no-op); the rendering-side dissolve
		// effect for Player is wired separately if needed.
		m_pAttackable = AddComponent<Attackable>("attackable",
			m_iInitHP, m_iInitAttackMin, m_iInitAttackMax);

		if (m_pTransform)
		{
			m_pTransform->SetPosition(10.f, 5.f, 10.f);
			m_pTransform->SetScale(0.01f, 0.01f, 0.01f);
			// Idle.mesh was authored in a Z-up convention; the engine is
			// Y-up, so rotate -90° around X once to stand the model upright.
			m_pTransform->SetRX(-PI / 2.f);
		}

		std::shared_ptr<Engine::Mesh> pMesh =
			Engine::StaticFindBindable<Engine::Mesh>("Idle");

		if (pMesh)
			pMesh->UsePaperBurn();

		if (m_pMeshRenderer)
		{
			m_pMeshRenderer->SetMesh(pMesh);
			m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_ANIM_VS));
			m_pMeshRenderer->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>(STANDARD_PS));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::DepthStencilState>("OutLineMask"));
			m_pMeshRenderer->SetAnimation(m_pAnimation);

			// Player stays in the normal opaque pass (correctly occluded by
			// voxels in the G-buffer), but also draws to the CustomDepth
			// target so the post-opaque composite can overlay a silhouette
			// wherever the player is hidden behind voxel terrain.
			m_pMeshRenderer->EnableCustomDepth(true);
		}

		std::shared_ptr<Engine::Animation> pAnimation = m_pAnimation;

		std::vector<std::string> vecSeq = {
			"Idle",
			"Run",
			"Attack",
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

				pAnimation->AddSequance(vecSeq[i], pSequence);
			}
		}

		std::shared_ptr<Engine::Skeleton> pSkeleton = Engine::ResourceManager::GetInst()->FindSkeleton("Idle");

		assert(pSkeleton);

		pAnimation->SetSkeleton(pSkeleton);

		//pAnimation->ChangeSequence("CharacterArmature|Idle");

		pAnimation->SetLoop("Idle");
		pAnimation->SetLoop("Run");
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

		m_pBody = AddComponent<Engine::ColliderOBB>("PlayerBody");

		m_pBody->SetScaleOffset({ 0.5f, 1.8f, 0.4f });

		m_pBody->SetAxisOffset({ 0.f, 0.9f, -0.1f });

		m_pBody->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Player::CollisionPlayerBodyStay);

		m_pCamera = Engine::Graphics::GetInst()->GetCamera();

		// Top-down: 60° pitch down from horizontal, camera sits high & behind
		// (+Z) on the player. Rotation is set once and never changed (no
		// mouse-look in this scene); world position is refreshed each frame
		// in Input via the standard "player - camAxisZ * dist" follow.
		const float fCamPitch = PI / 3.f;
		m_fCameraDist = 14.f;
		m_pCamera->GetTransform()->SetRelativeRotation(fCamPitch, PI, 0.f);

		m_pCameraLine = m_pCamera->CreateComponent<Engine::ColliderLine>("cameraline");

		m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &Player::CollisionCameraLine);
		m_pCameraLine->SetCallBack(Engine::COLLISION_TYPE::LAST, this, &Player::CollisionCameraLineLast);

		// Phase E5 — Trail is a Component now. Host it on a generic
		// GameObject so the Layer drives its lifecycle.
		std::shared_ptr<Engine::GameObject> pTrailObj =
			GetScene()->CreateGameObject<>("Trail", GetScene()->FindLayer(DEFAULT_LAYER));
		m_pTrail = pTrailObj ? pTrailObj->AddComponent<Trail>("trail", 10) : nullptr;

		/*for (int i = 0; i < 77; ++i)
		{
			std::string strNotify = "trail";

			strNotify += std::to_string(i);

			std::shared_ptr<Engine::Notify> pNotify = m_pAnimation->AddNotify("CharacterArmature|Sword_Slash", strNotify, i * 0.01666f);

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
		if (m_pTransform) m_pTransform->SetY(10.f);

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
				if (m_pAttackable) m_pAttackable->StartPaperBurn();
			}
		);

		Engine::CInput::GetInst()->AddKey(DIK_LCONTROL);

		m_pFootLSound = AddComponent<Engine::SoundBindable>("step_rock_l", "step_rock_l");

		m_pFootRSound = AddComponent<Engine::SoundBindable>("step_rock_r", "step_rock_r");

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

		// Seed both state machines so Update has something to dispatch on
		// the first frame. Anim sync in ChangeLowerState picks the Idle clip.
		ChangeLowerState(std::make_unique<PlayerLowerIdleState>());
		ChangeUpperState(std::make_unique<PlayerUpperIdleState>());

		return true;
	}

	void Player::Input(float fDeltaTime)
	{
		std::shared_ptr<Engine::Transform> pTransform = m_pTransform;
		std::shared_ptr<Engine::Transform> pCamTransform = m_pCamera->GetTransform();

		// Top-down WASD — world-axis aligned, but the camera sits at +Z and
		// looks -Z (yaw=PI), so "up on screen" is world -Z and "right on
		// screen" is world -X. Map W/D to the negative side accordingly.
		Engine::Vector3 vDir = {};
		if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_W)) { vDir.z -= 1.f; m_eDir = MOVE_DIR::UP; }
		if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_S)) { vDir.z += 1.f; m_eDir = MOVE_DIR::DOWN; }
		if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_D)) { vDir.x -= 1.f; m_eDir = MOVE_DIR::RIGHT; }
		if (Engine::CInput::GetInst()->IsKey(Engine::CInput::KEY_STATE::PRESS, DIK_A)) { vDir.x += 1.f; m_eDir = MOVE_DIR::LEFT; }

		// Voxel surface lookup. yMax is generous (covers any sensible stack).
		const int yMin = 0;
		const int yMax = 64;
		// The model's pivot sits at the character's waist (not the feet), so
		// "stand on top of block" needs an extra half-height lift past the
		// surface top. ~1 unit matches the 2-unit-tall Idle.mesh @ scale 0.01.
		const float fFeetOffset = 1.f;

		const Engine::Vector3 vCurr = pTransform->GetPosition();
		const int cx = static_cast<int>(std::floor(vCurr.x));
		const int cz = static_cast<int>(std::floor(vCurr.z));
		int currSurf = m_pVoxelWorld
			? m_pVoxelWorld->GetSurfaceHeight(cx, cz, yMin, yMax)
			: -1;

		// F: place a block at the player's standing cell (surface + 1) so
		// the next-frame surface snap lifts the player one block up.
		// G: clear the surface block under the player so they drop one cell.
		if (m_pVoxelWorld)
		{
			auto* pInput = Engine::CInput::GetInst();
			if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_F))
			{
				m_pVoxelWorld->SetBlock(cx, currSurf + 1, cz, Engine::BlockType::Stone);
				currSurf = m_pVoxelWorld->GetSurfaceHeight(cx, cz, yMin, yMax);
			}
			else if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_G) && currSurf >= 0)
			{
				m_pVoxelWorld->SetBlock(cx, currSurf, cz, Engine::BlockType::Air);
				currSurf = m_pVoxelWorld->GetSurfaceHeight(cx, cz, yMin, yMax);
			}
		}

		if (vDir != 0.f && m_pVoxelWorld)
		{
			vDir.Normalize();
			const Engine::Vector3 vTarget = vCurr + vDir * (fDeltaTime * m_fSpeed);
			const int tx = static_cast<int>(std::floor(vTarget.x));
			const int tz = static_cast<int>(std::floor(vTarget.z));
			const int tgtSurf = m_pVoxelWorld->GetSurfaceHeight(tx, tz, yMin, yMax);

			// Auto-step: allow 1-block ascent. Anything >1 is blocked.
			if (tgtSurf - currSurf <= 1)
			{
				pTransform->SetPosition(vTarget.x,
					static_cast<float>(tgtSurf + 1) + fFeetOffset,
					vTarget.z);
				// After SetRX(-PI/2) the model's natural forward becomes world
				// -Z (RY=0), so face-direction yaw is atan2 of the negated dir.
				pTransform->SetRY(atan2f(-vDir.x, -vDir.z));
				ChangeLowerState(std::make_unique<PlayerLowerRunState>());
			}
			else
			{
				// Blocked — re-anchor Y to current surface, hold idle.
				pTransform->SetY(static_cast<float>(currSurf + 1) + fFeetOffset);
				ChangeLowerState(std::make_unique<PlayerLowerIdleState>());
			}
		}
		else
		{
			if (m_pVoxelWorld)
				pTransform->SetY(static_cast<float>(currSurf + 1) + fFeetOffset);
			ChangeLowerState(std::make_unique<PlayerLowerIdleState>());
		}

		// Locked top-down camera follow — no mouse-look, no wheel zoom.
		Engine::Vector3 vCamPos = pTransform->GetPosition()
			- pCamTransform->GetAxis(Engine::AXIS_TYPE::Z) * m_fCameraDist;
		vCamPos.y += 1.2f;
		pCamTransform->SetPosition(vCamPos);

		m_pCameraLine->SetEndOffset(pCamTransform->GetAxis(Engine::AXIS_TYPE::Z));
	}

	void Player::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		// Top-down voxel mode: position/Y is fully owned by Input (snap-to-
		// surface). Gravity/terrain-fall logic was retired with the move to
		// voxels; UpdateState still runs so Idle/Run animations sync.
		UpdateState(fDeltaTime);

		// Periodic forward fire — emitter cadence. Multiple shots within a
		// single frame are possible if the frame stalled, so loop.
		m_fFireAcc += fDeltaTime;
		while (m_fFireAcc >= m_fFireInterval)
		{
			m_fFireAcc -= m_fFireInterval;
			//SpawnBullet();
		}
	}

	void Player::SpawnBullet()
	{
		if (!m_pTransform) return;

		// Player's world-forward at the current yaw. Derivation:
		//   model is authored Z-up; engine applies SetRX(-PI/2) once to
		//   stand it up, so model-local +Z maps to world -Y... then
		//   model-local +Y (the original "forward" of a Z-up rig) maps to
		//   world -Z at RY=0. Subsequent SetRY(θ) rotates that vector
		//   around world Y. Closed form: forward = (-sinθ, 0, -cosθ).
		const float fYaw = m_pTransform->GetRY();
		const Engine::Vector3 vForward(-sinf(fYaw), 0.f, -cosf(fYaw));

		// Spawn just in front of the player at knee height. The player
		// transform pivot sits at the waist (~surface + 2), so a -0.7u Y
		// offset puts the muzzle around y = surface + 1.3 — exactly the
		// centre of the voxel Enemy's body collider (cellY=1 → collider
		// sphere centred at y=1.3 with radius 0.35). Forward offset 0.6u
		// keeps the muzzle out of the player mesh.
		const Engine::Vector3 vSpawn =
			m_pTransform->GetPosition() + vForward * 0.6f + Engine::Vector3{ 0.f, -0.7f, 0.f };

		auto pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
		std::shared_ptr<Bullet> pBullet =
			GetScene()->CreateGameObject<Bullet>("bullet", pLayer);
		if (!pBullet) return;

		// Copy the player's rotation onto the bullet — same RX/RY chain
		// turns the bullet's local +Y (its movement axis in Bullet::Update)
		// into the player's world-forward direction.
		std::shared_ptr<Engine::Transform> pBulletTr = pBullet->GetTransform();
		if (pBulletTr)
		{
			pBulletTr->SetPosition(vSpawn);
			pBulletTr->SetRX(-PI / 2.f);
			pBulletTr->SetRY(fYaw);
		}

		// The particle trail lives on the Bullet itself (it owns and syncs
		// its own emitter Transform in Update). Player just sets the
		// bullet's pose; trail follows.
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
				if ((*iter).pGameObject) (*iter).pGameObject->InActivate();
				iter = m_ShadowList.erase(iter);
				iterEnd = m_ShadowList.end();
				continue;
			}
			std::shared_ptr<Engine::Mesh> pMesh =
				(*iter).pMeshRenderer ? (*iter).pMeshRenderer->GetMesh() : nullptr;

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
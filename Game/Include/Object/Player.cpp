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
#include "WeaponDatabase.h"
#include "PlayerState.h"
#include "Voxel/VoxelWorld.h"
#include "Scene/Scene.h"
#include "../Scene/GameScene.h"
#include <cmath>
#include <cstdlib>

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

	void Player::OnHitBy(Attackable* pAttacker)
	{
		if (!pAttacker || !m_pAttackable) return;

		if (pAttacker->Attack(m_pAttackable.get()))
			ChangeLowerState(std::make_unique<PlayerLowerDieState>());
		else
			ChangeLowerState(std::make_unique<PlayerLowerHitState>());
	}

	int Player::GetHP()    const { return m_pAttackable ? m_pAttackable->GetHP()    : 0; }
	int Player::GetMaxHP() const { return m_pAttackable ? m_pAttackable->GetMaxHP() : 1; }

	void Player::AddExp(int iAmount)
	{
		if (iAmount <= 0) return;
		m_iXp += iAmount;

		// Roll forward through as many boundaries as the pickup spans —
		// a single big AddExp shouldn't lose XP if it crosses two levels.
		// Threshold for level N is 5 * N (5 for L1→L2, 10 for L2→L3 …),
		// so progression slowly stretches per level.
		while (m_iXp >= m_iXpToNext)
		{
			m_iXp     -= m_iXpToNext;
			++m_iLevel;
			m_iXpToNext = 5 * m_iLevel;
			++m_iPendingLevelUps;   // counter so multi-level pickups queue cards
		}
	}

	void Player::ConsumeLevelUp(int iWeaponId)
	{
		// LevelUpChoices passes a CSV weapon id (the picked card). New
		// weapon ids open a fresh slot, existing ids bump the slot's
		// level in-place — either way one pending card is consumed.
		// The UI loops while PendingLevelUpCount() > 0 to drain a fat
		// pickup's queued cards before closing the modal.
		AddOrLevelUpWeapon(iWeaponId);
		if (m_iPendingLevelUps > 0) --m_iPendingLevelUps;
	}

	void Player::CollisionPlayerBodyStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (pDest->GetTag() == "orb_body")
		{
			// XP pickup — Enemy::OnCollision drops an Orb (tag "orb_body") at
			// its position on death. Route through AddExp so level-up
			// detection runs; Handling the pickup from the Player side
			// guarantees it fires even if the Orb's own collider callback
			// doesn't get dispatched.
			AddExp(1);
			if (auto* pOwner = pDest->GetGameObjectOwner())
				pOwner->InActivate();
			return;
		}

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
		// Player opts in to the blood particle (last arg true). Enemies /
		// monsters leave it at the default false so they skip the
		// per-frame CS dispatch + system-buffer upload for an emitter
		// they'd never visibly use.
		m_pAttackable = AddComponent<Attackable>("attackable",
			m_iInitHP, m_iInitAttackMin, m_iInitAttackMax, true);

		if (m_pTransform)
		{
			// 2D world — feet sit on top of the y=0 floor block at y=kWallY,
			// pivot at the waist is +1 above that. Input snaps Y each frame
			// regardless, but starting at the right height avoids the
			// first-frame drop.
			m_pTransform->SetPosition(10.f, static_cast<float>(kWallY) + 1.f, 10.f);
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

		pAnimation->SetLoop("Idle");
		pAnimation->SetLoop("Run");

		m_pBody = AddComponent<Engine::ColliderOBB>("PlayerBody");

		// Idle.mesh is Z-up and the transform applies SetRX(-PI/2) to stand
		// it upright, which makes model-local Y the world-forward axis and
		// model-local Z the world-up axis. Scale/axis offsets are expressed
		// in *local* space, so the "height" component must go on Z (not Y).
		//
		// The transform pivot sits at the waist (~surface + 2; feet at
		// surface + 1, head ~surface + 3), so an axis offset of (0,0,0)
		// centres a 1.8u-tall OBB at the waist, spanning [waist - 0.9,
		// waist + 0.9] ≈ knees-to-head. That overlaps the orb sphere
		// (centred ~0.3 above feet, radius 0.5) so a walking pickup fires
		// when the player's feet are roughly on top of the orb.
		m_pBody->SetScaleOffset({ 0.5f, 0.4f, 1.8f });

		m_pBody->SetAxisOffset({ 0.f, -0.1f, 0.f });

		// Pair filter — player body collides with enemies (so frogclaw-
		// style melee colliders register), bullets (none from enemies
		// today, kept for symmetry), and pickups (orbs).
		m_pBody->SetGroup(Engine::COLLISION_GROUP::PLAYER);
		m_pBody->SetMask(Engine::COLLISION_GROUP::ENEMY
		               | Engine::COLLISION_GROUP::BULLET
		               | Engine::COLLISION_GROUP::PICKUP);

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

		// LCTRL toggles bullet mouse-aim mode (see SpawnBullet). Must be
		// registered with CInput before Input() can poll its DOWN edge.
		// The duplicate AddKey above lives inside the 596–713 block
		// comment and never runs.
		Engine::CInput::GetInst()->AddKey(DIK_LCONTROL);

		// Seed both state machines so Update has something to dispatch on
		// the first frame. Anim sync in ChangeLowerState picks the Idle clip.
		ChangeLowerState(std::make_unique<PlayerLowerIdleState>());
		ChangeUpperState(std::make_unique<PlayerUpperIdleState>());

		// Grant a starting weapon so the player can damage enemies before
		// the first level-up card (no damage → no kills → no orbs → no
		// level-ups). Prefer the first weapon equipped in the combo
		// scene's loadout; fall back to Arrow (id=1) when nothing is
		// equipped so a straight-to-stage run isn't left weaponless.
		const std::vector<int> vecLoadout = WeaponDatabase::GetInst().EquippedLiveIds();
		AddOrLevelUpWeapon(vecLoadout.empty() ? 1 : vecLoadout.front());

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

		// 2D world: floor block at y=0 (top face at y=1), walls at y=kWallY.
		// Player feet sit on the floor's top face; the transform pivot is
		// at the waist (~1 unit above feet for the 2-unit-tall Idle.mesh),
		// so the player's Y is fixed at 2.0 for every frame.
		const float fFeetOffset = 1.f;
		const float fPlayerY    = static_cast<float>(kWallY) + fFeetOffset;

		const Engine::Vector3 vCurr = pTransform->GetPosition();
		const int cx = static_cast<int>(std::floor(vCurr.x));
		const int cz = static_cast<int>(std::floor(vCurr.z));

		// LCTRL toggles mouse-aim mode for bullets. DOWN edge so a
		// held key flips the state exactly once. SpawnBullet reads
		// m_bMouseAim and picks the aim source.
		{
			auto* pInput = Engine::CInput::GetInst();
			if (pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_LCONTROL))
			{
				m_bMouseAim = !m_bMouseAim;
			}
		}

		// F: place a wall in the cell immediately in front of the player.
		// G: remove the wall in the cell immediately in front of the player.
		// Placing at the player's own cell would trap them inside; the
		// "facing" cell is the natural target for build/break.
		if (m_pVoxelWorld)
		{
			auto* pInput = Engine::CInput::GetInst();
			const bool bF = pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_F);
			const bool bG = pInput->IsKey(Engine::CInput::KEY_STATE::DOWN, DIK_G);
			if (bF || bG)
			{
				// Player's world-forward at the current yaw (same derivation
				// as SpawnBullet): forward = (-sin θ, 0, -cos θ).
				const float fYaw = pTransform->GetRY();
				const float fxf  = -sinf(fYaw);
				const float fzf  = -cosf(fYaw);
				// Snap to the dominant axis so build/break always targets
				// exactly one neighbour cell (the visual model rotates in
				// 4 cardinal directions, so a diagonal forward would still
				// resolve to one cardinal neighbour).
				const int frontX = cx + (std::abs(fxf) > std::abs(fzf)
					? (fxf > 0.f ? 1 : -1) : 0);
				const int frontZ = cz + (std::abs(fzf) >= std::abs(fxf)
					? (fzf > 0.f ? 1 : -1) : 0);
				if (bF)
					m_pVoxelWorld->SetBlock(frontX, kWallY, frontZ, Engine::BlockType::Stone);
				else
					m_pVoxelWorld->SetBlock(frontX, kWallY, frontZ, Engine::BlockType::Air);
			}
		}

		if (vDir != 0.f && m_pVoxelWorld)
		{
			vDir.Normalize();
			const Engine::Vector3 vTarget = vCurr + vDir * (fDeltaTime * m_fSpeed);
			const int tx = static_cast<int>(std::floor(vTarget.x));
			const int tz = static_cast<int>(std::floor(vTarget.z));
			const bool bBlocked = Engine::IsSolid(
				m_pVoxelWorld->GetBlock(tx, kWallY, tz));

			if (!bBlocked)
			{
				pTransform->SetPosition(vTarget.x, fPlayerY, vTarget.z);
				// After SetRX(-PI/2) the model's natural forward becomes world
				// -Z (RY=0), so face-direction yaw is atan2 of the negated dir.
				pTransform->SetRY(atan2f(-vDir.x, -vDir.z));
				ChangeLowerState(std::make_unique<PlayerLowerRunState>());
			}
			else
			{
				// Blocked — hold position and switch to idle. Y still needs
				// to be re-anchored in case external code nudged it.
				pTransform->SetY(fPlayerY);
				ChangeLowerState(std::make_unique<PlayerLowerIdleState>());
			}
		}
		else
		{
			if (m_pVoxelWorld) pTransform->SetY(fPlayerY);
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

		// Weapon slots — Cooldown slots accumulate dt and fire when the
		// (level-scaled) cooldown is reached; Sustained slots have no
		// per-frame work here, their orbital instances tick themselves.
		for (auto& slot : m_vecWeaponSlots)
		{
			const WeaponDef* pDef = WeaponDatabase::GetInst().Get(slot.iWeaponId);
			if (!pDef || pDef->eFireMode != FireMode::Cooldown) continue;
			const float fCd = ComputeCooldown(*pDef, slot.iLevel);
			slot.fCooldownAcc += fDeltaTime;
			// Loop so a paused / hitching frame doesn't drop shots.
			int iGuard = 0;
			while (slot.fCooldownAcc >= fCd && iGuard++ < 8)
			{
				slot.fCooldownAcc -= fCd;
				FireCooldownBurst(slot);
			}
		}
	}

	float Player::ComputeAimYaw() const
	{
		if (!m_pTransform) return 0.f;
		// Default aim: player's world-forward at the current yaw.
		// Derivation: model is authored Z-up; engine applies SetRX(-PI/2)
		// once to stand it up, so model-local +Y (Z-up "forward") maps
		// to world -Z at RY=0. Subsequent SetRY(θ) rotates that vector
		// around world Y. Closed form: forward = (-sinθ, 0, -cosθ).
		float fAimYaw = m_pTransform->GetRY();

		// Mouse-aim override (LCTRL toggle). Cast a ray from the camera
		// through the mouse cursor onto the player's horizontal plane,
		// then derive the yaw whose forward direction points from the
		// player to that aim point. Falls back to the default yaw if any
		// engine resource is missing (no camera, near-vertical ray, etc.)
		// so we never spawn bullets with NaN orientations.
		if (m_bMouseAim && m_pCamera)
		{
			auto* pInput = Engine::CInput::GetInst();
			const Engine::Vector2 vMouseScreen{
				static_cast<float>(pInput->GetMouseX()),
				static_cast<float>(pInput->GetMouseY()) };
			const Engine::Vector3 vClip  = m_pCamera->ScreenPosToClipPos(vMouseScreen);
			// ScreenPosToClipPos maps screen Y as (screenY/H * 2 − 1), so
			// the top of the screen comes out as clip Y = −1 — opposite to
			// the D3D NDC convention CameraPosToWorldPos unprojects from
			// (NDC +Y is up). Flip Y here instead of fixing the shared
			// helper, since the UI/picking callers built around the
			// current convention and might break if it changes.
			const Engine::Vector3 vWorld = m_pCamera->CameraPosToWorldPos({ vClip.x, -vClip.y });
			const Engine::Vector3 vCamPos = m_pCamera->GetTransform()->GetPosition();

			const Engine::Vector3 vRayDir = vWorld - vCamPos;
			const Engine::Vector3 vPlayerPos = m_pTransform->GetPosition();

			// Solve cam.y + t*dir.y = player.y. With the locked top-
			// down camera the ray points clearly downward (dir.y < 0),
			// so |dir.y| is comfortably above the epsilon — but guard
			// anyway in case the camera ever flattens.
			if (std::abs(vRayDir.y) > 1e-4f)
			{
				const float t = (vPlayerPos.y - vCamPos.y) / vRayDir.y;
				if (t > 0.f)
				{
					const Engine::Vector3 vAim{
						vCamPos.x + vRayDir.x * t,
						vPlayerPos.y,
						vCamPos.z + vRayDir.z * t };
					const float dx = vAim.x - vPlayerPos.x;
					const float dz = vAim.z - vPlayerPos.z;
					if (dx * dx + dz * dz > 1e-6f)
					{
						// Inverse of the forward closed form above:
						// forward = (-sinθ, 0, -cosθ) gives
						// θ = atan2(-dx, -dz). Same convention used by
						// Player::Input when steering by WASD.
						fAimYaw = atan2f(-dx, -dz);
					}
				}
			}
		}

		return fAimYaw;
	}

	void Player::FireCooldownBurst(const WeaponSlot& slot)
	{
		if (!m_pTransform) return;
		const WeaponDef* pDef = WeaponDatabase::GetInst().Get(slot.iWeaponId);
		if (!pDef) return;

		const float fAimYaw = ComputeAimYaw();
		const Engine::Vector3 vForward(-sinf(fAimYaw), 0.f, -cosf(fAimYaw));
		const Engine::Vector3 vRight(cosf(fAimYaw), 0.f, -sinf(fAimYaw));
		const Engine::Vector3 vPlayerPos = m_pTransform->GetPosition();

		// Resolve the spawn anchor by the WeaponDef's origin. Front
		// matches the legacy bullet muzzle (kept the same offsets so
		// gameplay timing/aim doesn't shift); Around drops the projectile
		// straight at the player's pivot; Mouse projects the cursor onto
		// the player's y-plane and snaps the projectile there (used by
		// CursorShot — the projectile sits on the cursor and damages
		// whatever walks into it during its lifetime).
		Engine::Vector3 vSpawn = vPlayerPos + Engine::Vector3{ 0.f, kMuzzleYOffset, 0.f };
		switch (pDef->eOrigin)
		{
		case SpawnOrigin::Front:
			vSpawn = vSpawn + vForward * 0.6f;
			break;
		case SpawnOrigin::Around:
			break;
		case SpawnOrigin::Mouse:
			if (m_pCamera)
			{
				auto* pInput = Engine::CInput::GetInst();
				const Engine::Vector2 vMouseScreen{
					static_cast<float>(pInput->GetMouseX()),
					static_cast<float>(pInput->GetMouseY()) };
				const Engine::Vector3 vClip = m_pCamera->ScreenPosToClipPos(vMouseScreen);
				const Engine::Vector3 vWorld = m_pCamera->CameraPosToWorldPos({ vClip.x, -vClip.y });
				const Engine::Vector3 vCamPos = m_pCamera->GetTransform()->GetPosition();
				const Engine::Vector3 vRayDir = vWorld - vCamPos;
				if (std::abs(vRayDir.y) > 1e-4f)
				{
					const float t = (vPlayerPos.y - vCamPos.y) / vRayDir.y;
					if (t > 0.f)
					{
						vSpawn = {
							vCamPos.x + vRayDir.x * t,
							vPlayerPos.y + kMuzzleYOffset,
							vCamPos.z + vRayDir.z * t };
					}
				}
			}
			break;
		case SpawnOrigin::Random:
		{
			// A random point on the player's y-plane in a ring around them
			// (2..6 units), so area weapons rain down at unpredictable
			// spots near the player instead of from the muzzle.
			const float fAngle  = (static_cast<float>(std::rand()) / RAND_MAX) * (2.f * PI);
			const float fRadius = 2.f + (static_cast<float>(std::rand()) / RAND_MAX) * 4.f;
			vSpawn = vSpawn + Engine::Vector3{ cosf(fAngle) * fRadius, 0.f, sinf(fAngle) * fRadius };
			break;
		}
		default:
			break;
		}

		auto pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
		if (!pLayer) return;

		// Multi-shot burst — Count>1 fans projectiles around the aim
		// direction. ±10° per extra projectile feels tight enough to
		// stay readable.
		const int iCount = ComputeCount(*pDef, slot.iLevel);
		const float fFanStep = 0.174f;   // ~10°
		const float fFanBase = -fFanStep * (iCount - 1) * 0.5f;
		for (int i = 0; i < iCount; ++i)
		{
			auto pBullet = GetScene()->CreateGameObject<Bullet>("bullet", pLayer);
			if (!pBullet) continue;
			pBullet->Configure(*pDef, slot.iLevel, m_pTransform);
			// vSpawn already bakes kMuzzleYOffset for the first frame;
			// the orbital path also needs it to survive subsequent
			// Bullet::Update calls that re-anchor to the owner pivot.
			pBullet->SetOrbitYOffset(kMuzzleYOffset);
			if (auto pBulletTr = pBullet->GetTransform())
			{
				pBulletTr->SetPosition(vSpawn);
				pBulletTr->SetRX(-PI / 2.f);
				pBulletTr->SetRY(fAimYaw + fFanBase + fFanStep * i);
			}
		}
	}

	void Player::RespawnSustainedInstances(WeaponSlot& slot)
	{
		// Drop any still-live instances first — Orbital orbs from the
		// previous level no longer reflect the new count/speed/damage.
		for (auto& wp : slot.vecSustainedInstances)
			if (auto sp = wp.lock())
				sp->InActivate();
		slot.vecSustainedInstances.clear();

		if (!m_pTransform) return;
		const WeaponDef* pDef = WeaponDatabase::GetInst().Get(slot.iWeaponId);
		if (!pDef || pDef->eFireMode != FireMode::Sustained) return;

		auto pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
		if (!pLayer) return;

		const int iCount = ComputeCount(*pDef, slot.iLevel);
		// Same muzzle drop FireCooldownBurst uses — without it the orbs
		// would circle at player pivot height (~kWallY+1) and never
		// intersect enemy collider spheres (~kWallY+0.3).
		const Engine::Vector3 vSpawn =
			m_pTransform->GetPosition() + Engine::Vector3{ 0.f, kMuzzleYOffset, 0.f };
		// Evenly distribute starting angles around the player so multiple
		// orbs don't stack on top of each other. RY is interpreted as the
		// initial orbit angle in Bullet::Configure (Orbital path).
		for (int i = 0; i < iCount; ++i)
		{
			auto pBullet = GetScene()->CreateGameObject<Bullet>("bullet", pLayer);
			if (!pBullet) continue;
			if (auto pBulletTr = pBullet->GetTransform())
			{
				pBulletTr->SetPosition(vSpawn);
				pBulletTr->SetRX(-PI / 2.f);
				pBulletTr->SetRY((6.2831853f * i) / iCount);
			}
			pBullet->Configure(*pDef, slot.iLevel, m_pTransform);
			// Keep the orbital path at muzzle height — Bullet::Update
			// adds this each frame to vCenter.y (the owner pivot).
			pBullet->SetOrbitYOffset(kMuzzleYOffset);
			slot.vecSustainedInstances.emplace_back(pBullet);
		}
	}

	void Player::AddOrLevelUpWeapon(int iWeaponId)
	{
		const WeaponDef* pDef = WeaponDatabase::GetInst().Get(iWeaponId);
		if (!pDef)
		{
			MessageBox(nullptr, TEXT("Error"), TEXT("그런 무기가 없다."), MB_OK);
			assert(false);
			return;
		}

		// Existing slot → bump level. Sustained orbs need a re-spawn so a
		// count/speed bump on the level-up column actually takes effect;
		// Cooldown weapons just read the new level on the next fire.
		for (auto& slot : m_vecWeaponSlots)
		{
			if (slot.iWeaponId == iWeaponId)
			{
				++slot.iLevel;
				if (pDef->eFireMode == FireMode::Sustained)
					RespawnSustainedInstances(slot);
				return;
			}
		}

		// New weapon — capped at kMaxWeaponSlots. LevelUpChoices skips
		// new-weapon cards once the cap is hit, but the guard here keeps
		// the invariant local.
		if (static_cast<int>(m_vecWeaponSlots.size()) >= kMaxWeaponSlots) return;
		WeaponSlot slot;
		slot.iWeaponId = iWeaponId;
		slot.iLevel    = 1;
		m_vecWeaponSlots.push_back(std::move(slot));
		if (pDef->eFireMode == FireMode::Sustained)
			RespawnSustainedInstances(m_vecWeaponSlots.back());
	}

	std::vector<int> Player::GetOwnedWeaponIds() const
	{
		std::vector<int> out;
		out.reserve(m_vecWeaponSlots.size());
		for (const auto& s : m_vecWeaponSlots) out.push_back(s.iWeaponId);
		return out;
	}

	int Player::GetOwnedWeaponLevel(int iWeaponId) const
	{
		for (const auto& s : m_vecWeaponSlots)
			if (s.iWeaponId == iWeaponId) return s.iLevel;
		return 0;
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
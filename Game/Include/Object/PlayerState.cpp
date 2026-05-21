#include "PlayerState.h"
#include "Player.h"
#include "Attackable.h"
#include "Bullet.h"
#include "../UI/Inventory.h"
#include "Bindable/Animation.h"
#include "Bindable/Transform.h"
#include "Bindable/Terrain.h"
#include "Bindable/ColliderOBB.h"
#include "Animation/Sequence.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"

namespace Client
{
	// ── Lower body ───────────────────────────────────────────────────────

	std::unique_ptr<IState<Player>> PlayerLowerIdleState::Update(Player& /*p*/, float /*dt*/)
	{
		// Pure rest pose. Input handlers in Player::Input push Run/Roll/etc.
		return nullptr;
	}

	std::unique_ptr<IState<Player>> PlayerLowerRunState::Update(Player& /*p*/, float /*dt*/)
	{
		// Locomotion driven by Player::Input; Update has no auto-transition.
		return nullptr;
	}

	void PlayerLowerRollState::Enter(Player& p)
	{
		// Disable the body collider so the player passes through enemies
		// during the iframe roll. Animation rate is doubled via Player's
		// helper so the dodge feels snappier.
		if (auto pBody = p.GetBody()) pBody->Disable();
		p.SetRate(2.f);
	}

	void PlayerLowerRollState::Exit(Player& p)
	{
		// Restore normal anim rate; body collider re-enable lives in
		// Player::ChangeLowerState's Roll-exit gate (preserves the original
		// SetState semantic of re-enabling only when going to RollEnd/Die).
		p.SetRate(1.f);
		if (auto pBody = p.GetBody()) pBody->Enable();
	}

	std::unique_ptr<IState<Player>> PlayerLowerRollState::Update(Player& p, float fDeltaTime)
	{
		auto pAnim = p.GetAnimation();
		auto pTr   = p.GetTransformRef();
		auto pTerr = p.GetTerrain();
		if (!pAnim || !pTr) return nullptr;

		auto pSeq = pAnim->GetCurrentSequence();
		if (!pSeq || pSeq->GetTag() != "CharacterArmature|Roll")
		{
			return std::make_unique<PlayerLowerRollEndState>();
		}

		const Engine::Vector3& vPos     = pTr->GetPosition();
		const Engine::Vector3  vRollDir = p.GetRollDir();

		const float fHeight     = pTerr ? pTerr->GetTerrainHeight(vPos)            : 0.f;
		const float fNextHeight = pTerr ? pTerr->GetTerrainHeight(vPos + vRollDir) : 0.f;

		if (vPos.y >= fNextHeight || fHeight >= fNextHeight - tanf(PI / 4.f))
		{
			pTr->AddPosition(vRollDir * p.GetRollSpeed() * fDeltaTime);
			return nullptr;
		}
		return std::make_unique<PlayerLowerRollEndState>();
	}

	void PlayerLowerRollEndState::Enter(Player& /*p*/)
	{
		// No anim change — RollEnd lets the Roll clip's tail keep playing
		// until input pushes the player into Idle/Run on the next press.
	}

	std::unique_ptr<IState<Player>> PlayerLowerHitState::Update(Player& p, float /*dt*/)
	{
		auto pAnim = p.GetAnimation();
		if (!pAnim) return nullptr;

		auto pSeq = pAnim->GetCurrentSequence();
		if (!pSeq || pSeq->GetTag() != "CharacterArmature|HitRecieve")
			return std::make_unique<PlayerLowerHitEndState>();

		return nullptr;
	}

	// ── Upper body ───────────────────────────────────────────────────────

	void PlayerUpperAttackState::Enter(Player& p)
	{
		// Drive the weapon-specific additive sequence — matches the original
		// SetUpperBodyState(ATTACK) branch. Gun branch also spawns a bullet
		// GameObject; positioning is owed to a real equip path (post-Phase E5
		// the sword-tip socket is gone).
		auto pInv = p.GetInventory();
		if (!pInv) return;

		WEAPON_TYPE eWeapon = pInv->GetEquipWeaponType(Inventory::EQUIP_SLOT::HAND_RIGHT);
		switch (eWeapon)
		{
		case WEAPON_TYPE::FIST:
			p.SetAdditiveSequence("CharacterArmature|Punch_Left");
			break;
		case WEAPON_TYPE::SWORD:
			p.SetAdditiveSequence("CharacterArmature|Sword_Slash");
			break;
		case WEAPON_TYPE::GUN:
		{
			p.SetAdditiveSequence("CharacterArmature|Idle_Gun_Shoot");
			std::shared_ptr<Bullet> pBullet = p.GetScene()->CreateGameObject<Bullet>(
				"bullet", p.GetScene()->FindLayer(DEFAULT_LAYER));
			(void)pBullet;
		}
		break;
		default:
			break;
		}
	}

	std::unique_ptr<IState<Player>> PlayerUpperAttackState::Update(Player& p, float /*dt*/)
	{
		// Original UpdateState checked the additive sequence specifically
		// against Sword_Slash. Match that here so transitioning out on
		// non-sword attacks behaves identically to the previous code.
		auto pAnim = p.GetAnimation();
		if (!pAnim) return nullptr;

		auto pAdditive = pAnim->GetAdditiveSequence();
		if (!pAdditive || pAdditive->GetTag() != "CharacterArmature|Sword_Slash")
			return std::make_unique<PlayerUpperAttackEndState>();

		return nullptr;
	}
}

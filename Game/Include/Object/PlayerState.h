#pragma once

#include "State.h"

namespace Client
{
	class Player;

	// Player runs two independent state machines — a lower-body one (locomotion
	// + roll + hit + death) and an upper-body one (idle / attack). Splitting
	// the base interface lets StateMachine<Player, IPlayerLowerState> reject
	// upper-body states at compile time and vice versa.
	class IPlayerLowerState : public IState<Player> {};
	class IPlayerUpperState : public IState<Player> {};

	// ── Lower body states ────────────────────────────────────────────────

	class PlayerLowerIdleState : public IPlayerLowerState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& p, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Idle"; }
		const char* GetName() const override { return "Idle"; }
	};

	class PlayerLowerRunState : public IPlayerLowerState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& p, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Run"; }
		const char* GetName() const override { return "Run"; }
	};

	// Roll — disables the body collider on entry, restores anim rate on exit,
	// drives positional dodge in Update. Returns RollEnd when the Roll clip
	// finishes or the terrain ahead is too steep.
	class PlayerLowerRollState : public IPlayerLowerState
	{
	public:
		void Enter(Player& p) override;
		void Exit(Player& p)  override;
		std::unique_ptr<IState<Player>> Update(Player& p, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "CharacterArmature|Roll"; }
		const char* GetName() const override { return "Roll"; }
	};

	// Idle/transitional state between Roll completion and the next input.
	// Doesn't change anim (lets the Roll clip's tail keep playing until the
	// player presses something).
	class PlayerLowerRollEndState : public IPlayerLowerState
	{
	public:
		void Enter(Player& p) override;
		std::unique_ptr<IState<Player>> Update(Player& /*p*/, float /*dt*/) override
		{
			return nullptr;
		}
		const char* GetName() const override { return "RollEnd"; }
	};

	// Hit — locked while the HitRecieve animation plays. Auto-transitions to
	// HitEnd when the clip changes (matches original PlayerState::HIT logic).
	class PlayerLowerHitState : public IPlayerLowerState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& p, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "CharacterArmature|HitRecieve"; }
		const char* GetName() const override { return "Hit"; }
	};

	class PlayerLowerHitEndState : public IPlayerLowerState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& /*p*/, float /*dt*/) override
		{
			return nullptr;
		}
		const char* GetName() const override { return "HitEnd"; }
	};

	// Die — absorbing terminal state. OnHit also returns nullptr so a
	// post-death damage event can't bounce us back into Hit.
	class PlayerLowerDieState : public IPlayerLowerState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& /*p*/, float /*dt*/) override
		{
			return nullptr;
		}
		std::unique_ptr<IState<Player>> OnHit(Player& /*p*/, bool /*bLethal*/) override
		{
			return nullptr;
		}
		const char* GetAnimSequence() const override { return "CharacterArmature|Death"; }
		const char* GetName() const override { return "Die"; }
	};

	// ── Upper body states ────────────────────────────────────────────────

	class PlayerUpperIdleState : public IPlayerUpperState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& /*p*/, float /*dt*/) override
		{
			return nullptr;
		}
		const char* GetName() const override { return "UpperIdle"; }
	};

	// Attack — drives the weapon-specific additive sequence on entry. Update
	// polls the additive sequence and returns AttackEnd when it changes
	// (mirrors original SetUpperBodyState's ATTACK_END auto-transition).
	class PlayerUpperAttackState : public IPlayerUpperState
	{
	public:
		void Enter(Player& p) override;
		std::unique_ptr<IState<Player>> Update(Player& p, float fDeltaTime) override;
		const char* GetName() const override { return "UpperAttack"; }
	};

	class PlayerUpperAttackEndState : public IPlayerUpperState
	{
	public:
		std::unique_ptr<IState<Player>> Update(Player& /*p*/, float /*dt*/) override
		{
			return nullptr;
		}
		const char* GetName() const override { return "UpperAttackEnd"; }
	};
}

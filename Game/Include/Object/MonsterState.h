#pragma once

#include "State.h"

namespace Client
{
	class Monster;

	// Monster's concrete state node base. Just an alias for the generic
	// IState<Monster> so existing references read naturally and new
	// monster-specific states inherit a single named type. The state
	// pattern hookup (ChangeState, Update dispatch, OnHit forwarding)
	// lives in StateMachine<Monster>.
	using IMonsterState = IState<Monster>;

	// Idle — no movement, watch the player. Transition to Run/Attack on range.
	class MonsterIdleState : public IMonsterState
	{
	public:
		void Enter(Monster& m) override;
		std::unique_ptr<IMonsterState> Update(Monster& m, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Idle"; }
		const char* GetName() const override { return "Idle"; }
	};

	// Run — chase the player via the navmesh agent. Transition to Attack when
	// in melee range, Idle when player is too far.
	class MonsterRunState : public IMonsterState
	{
	public:
		void Enter(Monster& m) override;
		std::unique_ptr<IMonsterState> Update(Monster& m, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Run"; }
		const char* GetName() const override { return "Run"; }
	};

	// Attack — locked until the Attack animation finishes. Distance changes
	// during the swing are ignored on purpose (this was the entire reason
	// for converting the state machine to classes — the previous enum-based
	// version had Update force-switching on distance every frame).
	class MonsterAttackState : public IMonsterState
	{
	public:
		void Enter(Monster& m) override;
		void Exit(Monster& m) override;
		std::unique_ptr<IMonsterState> Update(Monster& m, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Attack"; }
		const char* GetName() const override { return "Attack"; }
	};

	// Hit — brief stagger. Time-driven so we don't rely on the "Jump"
	// animation having a notify at the end.
	class MonsterHitState : public IMonsterState
	{
	public:
		void Enter(Monster& m) override;
		std::unique_ptr<IMonsterState> Update(Monster& m, float fDeltaTime) override;
		const char* GetAnimSequence() const override { return "Jump"; }
		const char* GetName() const override { return "Hit"; }
	private:
		float m_fTimer = 0.f;
		static constexpr float kHitDuration = 0.5f;
	};

	// Die — absorbing terminal state. Update never returns a transition so
	// the monster stays dead. Agent and claw collider deactivated in Enter.
	class MonsterDieState : public IMonsterState
	{
	public:
		void Enter(Monster& m) override;
		std::unique_ptr<IMonsterState> Update(Monster& /*m*/, float /*dt*/) override
		{
			return nullptr;
		}
		std::unique_ptr<IMonsterState> OnHit(Monster& /*m*/, bool /*bLethal*/) override
		{
			return nullptr;
		}
		const char* GetAnimSequence() const override { return "Death"; }
		const char* GetName() const override { return "Die"; }
	};
}

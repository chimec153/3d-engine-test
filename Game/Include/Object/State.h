#pragma once

#include <memory>

namespace Client
{
	// Generic State Pattern interface — owner-typed so concrete states reach
	// the owner's API directly without dynamic_cast. Used by Monster, Player,
	// and any future GameObject that wants an explicit state machine.
	//
	// Conventions:
	//   - Update returns a new state to swap to, or nullptr to stay.
	//   - GetAnimSequence empty string means "don't change the anim on enter"
	//     (e.g. a transition state that keeps the previous clip).
	//   - Per-event hooks (OnHit etc.) default to nullptr (stay).
	template <typename TOwner>
	class IState
	{
	public:
		virtual ~IState() = default;

		virtual void Enter(TOwner& /*owner*/) {}
		virtual void Exit(TOwner&  /*owner*/) {}

		virtual std::unique_ptr<IState<TOwner>> Update(TOwner& owner, float fDeltaTime) = 0;

		virtual std::unique_ptr<IState<TOwner>> OnHit(TOwner& /*owner*/, bool /*bLethal*/)
		{
			return nullptr;
		}

		virtual const char* GetAnimSequence() const { return ""; }
		virtual const char* GetName() const = 0;
	};

	// Hosts a single IState<TOwner>. Drives the Exit→swap→Enter dance so
	// callers don't repeat that boilerplate. Owner reference is captured by
	// the host class once at construction and reused for every dispatch.
	//
	// The state machine intentionally does NOT touch animation/sound — it's
	// pure state ownership + dispatch. Owner-specific side effects (anim
	// sequence change, agent stop, etc.) belong in the state's Enter/Exit
	// implementations so a different owner can layer different effects on
	// the same state-machine plumbing.
	// TStateBase defaults to IState<TOwner> for the simple "one state tree
	// per owner" case (Monster). Multi-machine owners (Player has separate
	// lower-body and upper-body machines) instantiate StateMachine with a
	// narrower base — e.g. StateMachine<Player, IPlayerLowerState> — so the
	// type system catches an upper-body state being pushed into the lower
	// machine.
	template <typename TOwner, typename TStateBase = IState<TOwner>>
	class StateMachine
	{
	public:
		explicit StateMachine(TOwner& owner) : m_owner(owner) {}

		// Swap in a new state. Calls Exit on the current state (if any) and
		// Enter on the new state. Null is a no-op.
		void ChangeState(std::unique_ptr<TStateBase> pNext)
		{
			if (!pNext) return;
			if (m_pCurrent) m_pCurrent->Exit(m_owner);
			m_pCurrent = std::move(pNext);
			m_pCurrent->Enter(m_owner);
		}

		// Per-frame tick. If the current state's Update returns a non-null
		// successor, swap to it immediately so the next call sees the new
		// state. Returning early on the swap means OnHit-style events that
		// fired during this same frame still target the prior state — that
		// matches expected state-pattern semantics.
		void Update(float fDeltaTime)
		{
			if (m_pCurrent)
			{
				if (auto pNext = m_pCurrent->Update(m_owner, fDeltaTime))
					ChangeState(std::move(pNext));
			}
		}

		// Forward an external event to the active state. Used for damage,
		// collision callbacks, input edges, etc. — anywhere the trigger
		// originates outside Update.
		void OnHit(bool bLethal)
		{
			if (m_pCurrent)
			{
				if (auto pNext = m_pCurrent->OnHit(m_owner, bLethal))
					ChangeState(std::move(pNext));
			}
		}

		TStateBase* GetCurrent() const { return m_pCurrent.get(); }
		const char* GetCurrentName() const
		{
			return m_pCurrent ? m_pCurrent->GetName() : "(none)";
		}
		const char* GetCurrentAnimSequence() const
		{
			return m_pCurrent ? m_pCurrent->GetAnimSequence() : "";
		}

	private:
		TOwner& m_owner;
		std::unique_ptr<TStateBase> m_pCurrent;
	};
}

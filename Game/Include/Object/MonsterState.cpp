#include "MonsterState.h"
#include "Monster.h"
#include "Bindable/Transform.h"
#include "Bindable/Agent.h"
#include "Bindable/Animation.h"
#include "Bindable/ColliderOBB.h"
#include "Animation/Sequence.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"

namespace
{
	// Range thresholds — kept as a single source of truth so all three
	// states agree on where the boundaries are. If a state used a different
	// number, the monster would flicker between states on the edge.
	constexpr float kAttackRange = 1.f;
	constexpr float kChaseRange  = 10.f;

	// Returns the player's Transform if it's findable in DEFAULT_LAYER.
	// Centralised so each state's logic doesn't re-implement the lookup.
	std::shared_ptr<Engine::Transform> FindPlayerTransform(Client::Monster& m)
	{
		Engine::Scene* pScene = m.GetScene();
		if (!pScene) return nullptr;
		auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
		if (!pLayer) return nullptr;
		auto pPlayer = pLayer->FindGameObject("player");
		if (!pPlayer) return nullptr;
		return pPlayer->GetComponent<Engine::Transform>();
	}

	// Distance from monster to player. Returns FLT_MAX if either side
	// isn't found (callers treat that as "too far" → no aggro).
	float DistanceToPlayer(Client::Monster& m)
	{
		auto pSelfTr = m.GetTransform();
		if (!pSelfTr) return FLT_MAX;
		auto pPlayerTr = FindPlayerTransform(m);
		if (!pPlayerTr) return FLT_MAX;
		return (pSelfTr->GetPosition() - pPlayerTr->GetPosition()).Length();
	}
}

namespace Client
{
	// ── Idle ─────────────────────────────────────────────────────────────
	void MonsterIdleState::Enter(Monster& m)
	{
		// Park the agent on the monster's current position so navmesh
		// pathing doesn't try to walk anywhere.
		if (auto pAgent = m.GetAgent())
		{
			if (auto pTr = m.GetTransform())
				pAgent->SetTargetPos(pTr->GetPosition());
		}
	}

	std::unique_ptr<IMonsterState> MonsterIdleState::Update(Monster& m, float)
	{
		const float fDist = DistanceToPlayer(m);
		if (fDist <= kAttackRange) return std::make_unique<MonsterAttackState>();
		if (fDist <= kChaseRange)  return std::make_unique<MonsterRunState>();
		return nullptr;
	}

	// ── Run ──────────────────────────────────────────────────────────────
	void MonsterRunState::Enter(Monster& /*m*/) {}

	std::unique_ptr<IMonsterState> MonsterRunState::Update(Monster& m, float)
	{
		const float fDist = DistanceToPlayer(m);
		if (fDist <= kAttackRange) return std::make_unique<MonsterAttackState>();
		if (fDist >  kChaseRange)  return std::make_unique<MonsterIdleState>();

		// Refresh the agent's destination every frame; player may have moved.
		if (auto pAgent = m.GetAgent())
		{
			if (auto pPlayerTr = FindPlayerTransform(m))
				pAgent->SetTargetPos(pPlayerTr->GetPosition());
		}
		return nullptr;
	}

	// ── Attack ───────────────────────────────────────────────────────────
	void MonsterAttackState::Enter(Monster& m)
	{
		// Make sure stale "AttackEnd" flag from a previous attack doesn't
		// short-circuit this swing.
		m.ResetAttackFinished();

		// Freeze the agent — chase logic must not run while we're attacking.
		// The whole point of using state classes here: Update no longer
		// rebinds the agent target on distance change.
		if (auto pAgent = m.GetAgent())
		{
			if (auto pTr = m.GetTransform())
				pAgent->SetTargetPos(pTr->GetPosition());
		}
	}

	void MonsterAttackState::Exit(Monster& /*m*/) {}

	std::unique_ptr<IMonsterState> MonsterAttackState::Update(Monster& m, float)
	{
		// Primary signal: AttackEnd notify (set by the 0.95-time notify in
		// Monster::Init) sets the finished flag, which we consume here.
		bool bFinished = m.ConsumeAttackFinished();

		// Belt-and-braces fallback. If the notify never fires (clip renamed,
		// notify time past clip length, sequence not registered under the
		// expected alias, etc.) we would otherwise be stuck in Attack
		// forever — exactly the "공격 후 멈춤" symptom. Probe the Animation
		// component: once the current clip is no longer "Attack" (the anim
		// system auto-advanced or the user wired SetNextSequence), or
		// the current sequence is null, treat that as end-of-attack too.
		if (!bFinished)
		{
			if (auto pAnim = m.GetAnimation())
			{
				auto pSeq = pAnim->GetCurrentSequence();
				if (!pSeq || pSeq->GetTag() != "Attack")
					bFinished = true;
			}
		}

		if (!bFinished) return nullptr;

		// Decide the follow-up state from current range.
		const float fDist = DistanceToPlayer(m);
		if (fDist <= kAttackRange) return std::make_unique<MonsterAttackState>();
		if (fDist <= kChaseRange)  return std::make_unique<MonsterRunState>();
		return std::make_unique<MonsterIdleState>();
	}

	// ── Hit ──────────────────────────────────────────────────────────────
	void MonsterHitState::Enter(Monster& m)
	{
		m_fTimer = 0.f;
		// Stop in place during the stagger.
		if (auto pAgent = m.GetAgent())
		{
			if (auto pTr = m.GetTransform())
				pAgent->SetTargetPos(pTr->GetPosition());
		}
	}

	std::unique_ptr<IMonsterState> MonsterHitState::Update(Monster& /*m*/, float fDeltaTime)
	{
		m_fTimer += fDeltaTime;
		if (m_fTimer >= kHitDuration)
			return std::make_unique<MonsterIdleState>();
		return nullptr;
	}

	// ── Die ──────────────────────────────────────────────────────────────
	void MonsterDieState::Enter(Monster& m)
	{
		// Disable everything that could still drive movement or damage.
		if (auto pAgent = m.GetAgent()) pAgent->InActivate();
		if (auto pClaw  = m.GetClawBody()) pClaw->InActivate();
	}
}

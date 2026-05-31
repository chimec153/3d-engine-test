#include "EnemySpawner.h"
#include "GameWorldBuilder.h"
#include "../Object/Enemy.h"
#include "../Object/AggroTarget.h"
#include "../Object/EnemyData.h"
#include "../Object/EnemyDatabase.h"
#include "../Object/FlowField.h"
#include "../Object/RoundDatabase.h"
#include "../Object/Vfx/SpawnTelegraphManager.h"
#include "../GameDefs.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Bindable/Transform.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "Types.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Client
{
    namespace EnemySpawner_detail
    {
        // Random angle in [0, 2π). std::rand is the project-wide PRNG choice
        // (the codebase forbids <random> due to a global "epsilon" macro that
        // mangles MSVC's int128 headers — see engine_epsilon_macro_breaks_random
        // memory).
        float RandAngle()
        {
            return (std::rand() / static_cast<float>(RAND_MAX)) * 2.f * PI;
        }

        // Returns the world-space (x,z) for one slot of a burst placement.
        //   EdgeRandom   — one shared random angle for the whole burst (all
        //                  slots land near the same cell). Caller jitters
        //                  individual cells if needed.
        //   EdgeAll      — slots evenly spaced around 360° at kEdgeRadius.
        //   PointBurst   — single random angle, slots cluster in a small
        //                  patch around that angle (more visceral "burst").
        //   Ring         — evenly spaced around 360° at kRingRadius (closer
        //                  to the player than EdgeAll).
        // The base random angle is provided by the caller (one per burst)
        // so all slots of a single burst share it without re-rolling.
        void PlaceSlot(SpawnPattern ePattern, int iSlot, int iBurst,
                       float fBurstAngle, float fRadius,
                       const Engine::Vector3& vPlayer,
                       float& fX, float& fZ)
        {
            float fAngle = fBurstAngle;

            switch (ePattern)
            {
            case SpawnPattern::EdgeRandom:
                // All slots share fBurstAngle; small per-slot jitter so a
                // burst > 1 doesn't stack atom-on-atom.
                fAngle += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 0.3f;
                break;
            case SpawnPattern::EdgeAll:
            case SpawnPattern::Ring:
                fAngle = (static_cast<float>(iSlot) / (std::max)(1, iBurst)) * 2.f * PI;
                break;
            case SpawnPattern::PointBurst:
                // Tight cluster offset around the burst angle (each slot a
                // small angular nudge, ~2-cell-wide cluster at fRadius).
                fAngle += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 0.5f;
                break;
            }
            fX = vPlayer.x + std::cos(fAngle) * fRadius;
            fZ = vPlayer.z + std::sin(fAngle) * fRadius;
        }
    }

    EnemySpawner::EnemySpawner(Engine::Scene* pScene, Engine::VoxelWorld* pWorld)
        : m_pScene(pScene), m_pWorld(pWorld),
          m_pFlowField(std::make_unique<FlowField>())
    {
    }

    EnemySpawner::~EnemySpawner() = default;

    void EnemySpawner::Tick(float fDeltaTime)
    {
        if (!m_pScene || !m_pWorld) return;

        // Maintain the shared flow field every tick — independent of the
        // spawn cadence / window so enemies still get a current field
        // even while spawning is paused (grace period, hard-stop reached).
        auto pLayer = m_pScene->FindLayer(DEFAULT_LAYER);
        std::shared_ptr<Engine::GameObject> pPlayer;
        std::shared_ptr<Engine::Transform>  pPlayerTr;
        if (pLayer)
        {
            pPlayer = pLayer->FindGameObject("player");
            if (pPlayer) pPlayerTr = pPlayer->GetComponent<Engine::Transform>();
        }
        std::shared_ptr<Engine::GameObject> pGoal;
        if (m_pFlowField && pLayer)
        {
            // Towers are unbreakable to pathing — collect their cells so the
            // field routes around them (and a target boxed in by towers reads
            // as unreachable to the enemies outside).
            std::vector<std::pair<int, int>> vecBlocked;
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive()) continue;
                if (p->GetTag() != "Tower" && p->GetTag() != "HealTower") continue;
                auto pTr = p->GetComponent<Engine::Transform>();
                if (!pTr) continue;
                const Engine::Vector3 v = pTr->GetPosition();
                vecBlocked.emplace_back(
                    static_cast<int>(std::floor(v.x)),
                    static_cast<int>(std::floor(v.z)));
            }

            // Reachability probe: a representative live enemy. The goal must be
            // reachable from where the enemies actually are — NOT from the
            // player, who may be walled inside a tower fortress. SelectReachableGoal
            // then returns the strongest-aggro target that side of the walls can
            // reach: normally the protected target, but the nearest wall tower
            // when the real one is boxed in. Enemies path to it and melee it;
            // once it dies the blocked set changes, the field rebuilds, and they
            // push further in.
            int probeCx = 0, probeCz = 0;
            const bool bProbe = FindProbeEnemy(pLayer.get(), probeCx, probeCz);

            pGoal = SelectReachableGoal(pLayer.get(), vecBlocked,
                                        probeCx, probeCz, bProbe, pPlayer);
        }
        else
        {
            pGoal = pPlayer;
        }
        if (pGoal && pLayer)
        {
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
                if (auto pEnemy = std::dynamic_pointer_cast<Enemy>(p))
                    pEnemy->SetTarget(pGoal);
            }
        }

        // Round ended (or never started) → drop any in-flight telegraphs so
        // they don't materialise into the intermission / next round.
        if (!m_bRoundActive || !m_pCurrentRound)
        {
            m_vecPending.clear();
            return;
        }

        // Advance the round timer even if we can't spawn this frame (so the
        // round still ends on time when the field is full / player is gone).
        m_fRoundElapsed += fDeltaTime;

        if (!pLayer || !pPlayer || !pPlayerTr) return;

        // 1) Tick existing telegraphs: age, submit ground VFX, materialise
        //    any that completed this frame.
        for (auto it = m_vecPending.begin(); it != m_vecPending.end(); )
        {
            it->fAge += fDeltaTime;
            const float fFill = (it->fAge >= kTelegraphTime)
                ? 1.f : (it->fAge / kTelegraphTime);
            const Engine::Vector3 vCentre(
                static_cast<float>(it->cx) + 0.5f,
                static_cast<float>(kWallY) + 0.02f,
                static_cast<float>(it->cz) + 0.5f);
            SpawnTelegraphManager::GetInst()->Submit(vCentre, kTelegraphRadius, fFill);

            if (it->fAge >= kTelegraphTime)
            {
                MaterialisePending(*it, pPlayer, pLayer);
                it = m_vecPending.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 1.5) Boss spawn — one shot, at fSpawnTime, no telegraph. The
        //      boss is materialised straight into the world (point_burst
        //      with a fallback through several angles for wall avoidance).
        if (m_pCurrentRound->bIsBoss && !m_bBossSpawned &&
            m_fRoundElapsed >= m_pCurrentRound->tBoss.fSpawnTime)
        {
            MaterialiseBoss(pPlayer, pLayer, pPlayerTr);
            m_bBossSpawned = true;
        }

        // 2) Advance each spawn entry's window + accumulator and queue
        //    bursts on every elapse. The state vector is grown to match
        //    vecSpawns at StartRound, but a defensive resize keeps Tick
        //    safe if someone forgot.
        const auto& vecSpawns = m_pCurrentRound->vecSpawns;
        if (m_vecSpawnStates.size() < vecSpawns.size())
            m_vecSpawnStates.resize(vecSpawns.size());

        for (size_t i = 0; i < vecSpawns.size(); ++i)
        {
            const RoundSpawn& s = vecSpawns[i];
            // Inside this spawn entry's [startTime, endTime) window?
            if (m_fRoundElapsed < s.fStartTime)  continue;
            if (m_fRoundElapsed >= s.fEndTime)   continue;
            if (s.fInterval <= 0.f)              continue;   // malformed row

            SpawnState& st = m_vecSpawnStates[i];
            st.fAcc += fDeltaTime;
            while (st.fAcc >= s.fInterval)
            {
                st.fAcc -= s.fInterval;

                // Concurrent cap covers live enemies AND queued telegraphs
                // so a flood of warnings can't push the field past the cap
                // once they materialise.
                const int iInFlight = GetAliveEnemyCount() + static_cast<int>(m_vecPending.size());
                if (iInFlight >= kMaxAliveEnemies) break;

                for (int slot = 0; slot < s.iBurst; ++slot)
                {
                    QueueOne(s, slot, pPlayerTr);
                }
            }
        }
    }

    bool EnemySpawner::FindProbeEnemy(Engine::Layer* pLayer, int& cx, int& cz) const
    {
        if (!pLayer) return false;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            const Engine::Vector3& v = pTr->GetPosition();
            cx = static_cast<int>(std::floor(v.x));
            cz = static_cast<int>(std::floor(v.z));
            return true;
        }
        return false;
    }

    std::shared_ptr<Engine::GameObject> EnemySpawner::SelectReachableGoal(
        Engine::Layer* pLayer,
        const std::vector<std::pair<int, int>>& vecBlocked,
        int probeCx, int probeCz, bool bProbe,
        const std::shared_ptr<Engine::GameObject>& pPlayer)
    {
        if (!pLayer || !m_pFlowField || !m_pWorld) return pPlayer;

        struct Cand
        {
            std::shared_ptr<Engine::GameObject> obj;
            int aggro;
            int cx, cz;
        };
        std::vector<Cand> cands;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            auto pAggro = p->GetComponent<AggroTarget>();
            if (!pAggro) continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            const Engine::Vector3& v = pTr->GetPosition();
            cands.push_back({ p, pAggro->GetAggro(),
                              static_cast<int>(std::floor(v.x)),
                              static_cast<int>(std::floor(v.z)) });
        }
        if (cands.empty()) return pPlayer;

        // Build the field to a candidate and report whether the enemies can
        // reach it. With no live enemy to probe, any candidate "works" — we
        // just keep a field current for the next spawns.
        const auto BuildReaches = [&](const Cand& c) -> bool
        {
            m_pFlowField->Rebuild(*m_pWorld, c.cx, c.cz, vecBlocked);
            return !bProbe || m_pFlowField->Reaches(probeCx, probeCz);
        };

        // 1) Stick with the committed goal while it's still a live, reachable
        //    candidate — re-searching every tick would cost a rebuild per
        //    candidate. (A stationary tower's Rebuild is a cached no-op here.)
        if (auto pCommitted = m_wpCommittedGoal.lock())
        {
            for (const auto& c : cands)
            {
                if (c.obj != pCommitted) continue;
                if (BuildReaches(c)) return pCommitted;
                break;
            }
        }

        // 2) Re-search: strongest aggro first, nearest to the enemies breaking
        //    ties so they swarm the closest reachable target (e.g. the nearest
        //    wall tower). First reachable candidate wins and is committed.
        std::sort(cands.begin(), cands.end(), [&](const Cand& a, const Cand& b)
        {
            if (a.aggro != b.aggro) return a.aggro > b.aggro;
            const long long da = static_cast<long long>(a.cx - probeCx) * (a.cx - probeCx)
                               + static_cast<long long>(a.cz - probeCz) * (a.cz - probeCz);
            const long long db = static_cast<long long>(b.cx - probeCx) * (b.cx - probeCx)
                               + static_cast<long long>(b.cz - probeCz) * (b.cz - probeCz);
            return da < db;
        });
        for (const auto& c : cands)
        {
            if (BuildReaches(c)) { m_wpCommittedGoal = c.obj; return c.obj; }
        }

        // 3) Nothing reachable (every aggro target walled off). Keep a field on
        //    the strongest target so freshly-spawned enemies have one; any
        //    already walled out simply hold until a wall opens.
        m_pFlowField->Rebuild(*m_pWorld, cands.front().cx, cands.front().cz, vecBlocked);
        m_wpCommittedGoal = cands.front().obj;
        return cands.front().obj;
    }

    void EnemySpawner::StartRound(int iRound)
    {
        m_iRound         = iRound;
        m_pCurrentRound  = RoundDatabase::GetInst().Get(iRound);
        m_fRoundElapsed  = 0.f;
        m_bRoundActive   = m_pCurrentRound != nullptr;
        m_bBossSpawned   = false;
        // Reset per-spawn accumulators so a fresh round starts cleanly.
        m_vecSpawnStates.clear();
        if (m_pCurrentRound)
            m_vecSpawnStates.resize(m_pCurrentRound->vecSpawns.size());
    }

    void EnemySpawner::EndRound()
    {
        m_bRoundActive  = false;
        m_pCurrentRound = nullptr;
    }

    int EnemySpawner::GetAliveEnemyCount() const
    {
        if (!m_pScene) return 0;
        auto pLayer = m_pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return 0;
        int iCount = 0;
        for (const auto& p : pLayer->GetGameObjectList())
            if (p && p->IsActive() && p->GetTag() == "Enemy") ++iCount;
        return iCount;
    }

    bool EnemySpawner::IsRoundComplete() const
    {
        if (!m_bRoundActive || !m_pCurrentRound) return false;
        return m_fRoundElapsed >= m_pCurrentRound->fDuration;
    }

    float EnemySpawner::GetRoundTimeRemaining() const
    {
        if (!m_bRoundActive || !m_pCurrentRound) return 0.f;
        const float fLeft = m_pCurrentRound->fDuration - m_fRoundElapsed;
        return fLeft > 0.f ? fLeft : 0.f;
    }

    void EnemySpawner::ClearEnemies()
    {
        m_vecPending.clear();

        if (!m_pScene) return;
        auto pLayer = m_pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;
        for (const auto& p : pLayer->GetGameObjectList())
            if (p && p->IsActive() && p->GetTag() == "Enemy")
                p->InActivate();
    }

    void EnemySpawner::QueueOne(const RoundSpawn& spawn, int iSlot,
                                const std::shared_ptr<Engine::Transform>& pPlayerTr)
    {
        using namespace EnemySpawner_detail;

        const Engine::Vector3 vPlayer = pPlayerTr->GetPosition();
        // One random base angle per burst — captured on the first slot and
        // reused for the rest. EdgeAll / Ring overwrite it inside PlaceSlot
        // with evenly-spaced angles, so the base is only used by the
        // single-point patterns.
        //
        // We don't have a "first slot of this burst" hook (QueueOne is called
        // in a flat loop), so we re-roll each call but the per-slot jitter
        // inside PlaceSlot ensures multi-slot bursts don't stack exactly.
        const float fBurstAngle = RandAngle();

        const int iMax = GameWorldBuilder::kFloorSize - 1;
        int  cx = 0, cz = 0;
        bool bOpen = false;
        const float fPatternRadius =
            (spawn.ePattern == SpawnPattern::Ring) ? kRingRadius : kEdgeRadius;
        for (int attempt = 0; attempt < 6 && !bOpen; ++attempt)
        {
            float fX = 0.f, fZ = 0.f;
            // Re-roll the per-slot angle each attempt — same slot index +
            // burst, so EdgeAll/Ring keep their evenly-spaced position
            // (deterministic in iSlot/iBurst), while EdgeRandom/PointBurst
            // get a fresh jitter on retry.
            PlaceSlot(spawn.ePattern, iSlot, spawn.iBurst,
                      fBurstAngle, fPatternRadius, vPlayer, fX, fZ);
            cx = (std::max)(1, (std::min)(iMax - 1, static_cast<int>(std::floor(fX))));
            cz = (std::max)(1, (std::min)(iMax - 1, static_cast<int>(std::floor(fZ))));
            bOpen = !Engine::IsSolid(m_pWorld->GetBlock(cx, kWallY, cz));
        }
        // Walled in on every attempt — skip this slot rather than queue a
        // telegraph on top of a wall cell.
        if (!bOpen) return;

        PendingSpawn pend{};
        pend.cx         = cx;
        pend.cz         = cz;
        pend.strEnemyId = spawn.strEnemyId;
        pend.fHpMult    = m_pCurrentRound ? m_pCurrentRound->fHpMultiplier     : 1.f;
        pend.fDmgMult   = m_pCurrentRound ? m_pCurrentRound->fDamageMultiplier : 1.f;
        m_vecPending.push_back(pend);
    }

    void EnemySpawner::MaterialiseBoss(const std::shared_ptr<Engine::GameObject>& pPlayer,
                                       const std::shared_ptr<Engine::Layer>& pLayer,
                                       const std::shared_ptr<Engine::Transform>& pPlayerTr)
    {
        if (!m_pCurrentRound || !pLayer || !pPlayerTr) return;
        const EnemyDef* pDef = EnemyDatabase::GetInst().Get(m_pCurrentRound->tBoss.strBossId);
        if (!pDef || !pDef->bIsBoss) return;

        const Engine::Vector3 vPlayer = pPlayerTr->GetPosition();
        const int iMax = GameWorldBuilder::kFloorSize - 1;
        int cx = 0, cz = 0;
        bool bOpen = false;
        for (int attempt = 0; attempt < 12 && !bOpen; ++attempt)
        {
            const float fAngle =
                (std::rand() / static_cast<float>(RAND_MAX)) * 2.f * PI;
            const float fX = vPlayer.x + std::cos(fAngle) * kEdgeRadius;
            const float fZ = vPlayer.z + std::sin(fAngle) * kEdgeRadius;
            cx = (std::max)(1, (std::min)(iMax - 1, static_cast<int>(std::floor(fX))));
            cz = (std::max)(1, (std::min)(iMax - 1, static_cast<int>(std::floor(fZ))));
            bOpen = !Engine::IsSolid(m_pWorld->GetBlock(cx, kWallY, cz));
        }
        if (!bOpen) return;

        // Multipliers apply to bosses' minions in their phases too — but the
        // boss itself doesn't double-dip on hpMultiplier (its baseHp is
        // already huge and authored against round 10/20). For now, apply
        // the same multipliers — round 10 was authored with 2.62×/1.9× so
        // an unscaled devourer is too easy; the JSON design notes assume
        // the curve still applies to the boss.
        EnemyDef def = *pDef;
        def.iMaxHP        = static_cast<int>(static_cast<float>(def.iBaseHp)        * m_pCurrentRound->fHpMultiplier  + 0.5f);
        if (def.iMaxHP < 1) def.iMaxHP = 1;
        const int iScaledMelee = static_cast<int>(
            static_cast<float>(def.iContactDamage) * m_pCurrentRound->fDamageMultiplier + 0.5f);
        def.iAttackMin = iScaledMelee;
        def.iAttackMax = iScaledMelee;

        auto pEnemy = m_pScene->CreateGameObject<Enemy>("Enemy", pLayer);
        if (!pEnemy) return;
        pEnemy->ApplyDef(def);
        pEnemy->SetVoxelWorld(m_pWorld);
        pEnemy->SetFlowField(m_pFlowField.get());
        pEnemy->SetSpawnCell(cx, cz);
        pEnemy->SetTarget(pPlayer);
    }

    void EnemySpawner::MaterialisePending(const PendingSpawn& pending,
                                          const std::shared_ptr<Engine::GameObject>& pPlayer,
                                          const std::shared_ptr<Engine::Layer>& pLayer)
    {
        if (m_pWorld && Engine::IsSolid(m_pWorld->GetBlock(pending.cx, kWallY, pending.cz)))
            return;

        const EnemyDef* pDef = EnemyDatabase::GetInst().Get(pending.strEnemyId);
        if (!pDef) return;   // unknown id — silently drop (data error)

        auto pEnemy = m_pScene->CreateGameObject<Enemy>("Enemy", pLayer);
        if (!pEnemy) return;

        // Per-spawn copy with round multipliers baked in. Enemy::ApplyDef
        // only reads the runtime fields (iMaxHP / iAttackMin / iAttackMax /
        // fSpeed), so applying the maths here means Enemy stays unaware of
        // the multiplier pipeline.
        EnemyDef def = *pDef;
        def.iMaxHP     = static_cast<int>(static_cast<float>(def.iBaseHp)        * pending.fHpMult  + 0.5f);
        if (def.iMaxHP < 1) def.iMaxHP = 1;
        const int iScaledDmg = static_cast<int>(
            static_cast<float>(def.iContactDamage) * pending.fDmgMult + 0.5f);
        def.iAttackMin = iScaledDmg;
        def.iAttackMax = iScaledDmg;
        // Behavior-special damage (projectile / explosion) scales with the
        // round damageMultiplier too, so a late-round bomber/spitter
        // ramps with the curve instead of staying at base damage.
        def.iProjDamage    = static_cast<int>(static_cast<float>(def.iProjDamage)    * pending.fDmgMult + 0.5f);
        def.iExplodeDamage = static_cast<int>(static_cast<float>(def.iExplodeDamage) * pending.fDmgMult + 0.5f);

        pEnemy->ApplyDef(def);
        pEnemy->SetVoxelWorld(m_pWorld);
        pEnemy->SetFlowField(m_pFlowField.get());
        pEnemy->SetSpawnCell(pending.cx, pending.cz);
        pEnemy->SetTarget(pPlayer);
    }
}

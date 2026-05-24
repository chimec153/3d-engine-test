#include "EnemySpawner.h"
#include "GameWorldBuilder.h"
#include "../Object/Enemy.h"
#include "../Object/EnemyDatabase.h"
#include "../Object/FlowField.h"
#include "../Object/SpawnConfig.h"
#include "../GameDefs.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Bindable/Transform.h"
#include "Types.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Client
{
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
        if (pPlayerTr && m_pFlowField)
        {
            const Engine::Vector3& vPos = pPlayerTr->GetPosition();
            const int gx = static_cast<int>(std::floor(vPos.x));
            const int gz = static_cast<int>(std::floor(vPos.z));
            // Rebuild only when the player crosses into a new cell.
            m_pFlowField->Rebuild(*m_pWorld, gx, gz);
        }

        // Advance the scene clock first — every rule's first/last gate
        // compares against this.
        m_fElapsed += fDeltaTime;

        // Iterate every SpawnRule in parallel. Each rule keeps its own
        // accumulator and round-robin index so two rules with different
        // intervals don't drag on each other (e.g. 0.5s Box wave and
        // 1.0s Capsule wave fire on independent cadences).
        const auto& vecRules = SpawnConfig::GetInst().GetRules();
        if (m_vecRuleStates.size() < vecRules.size())
            m_vecRuleStates.resize(vecRules.size());

        if (!pLayer || !pPlayer || !pPlayerTr) return;

        for (size_t i = 0; i < vecRules.size(); ++i)
        {
            const SpawnRule& rule = vecRules[i];
            RuleState& state = m_vecRuleStates[i];

            if (m_fElapsed < rule.fFirstTime) continue;                       // grace
            if (rule.fLastTime > 0.f && m_fElapsed >= rule.fLastTime) continue; // hard stop

            state.fSpawnAcc += fDeltaTime;
            if (state.fSpawnAcc < rule.fInterval) continue;
            state.fSpawnAcc -= rule.fInterval;

            SpawnOne(rule, state, pPlayer, pPlayerTr, pLayer);
        }
    }

    void EnemySpawner::SpawnOne(const SpawnRule& rule, RuleState& state,
                                const std::shared_ptr<Engine::GameObject>& pPlayer,
                                const std::shared_ptr<Engine::Transform>& pPlayerTr,
                                const std::shared_ptr<Engine::Layer>& pLayer)
    {
        const float fAngle =
            (std::rand() / static_cast<float>(RAND_MAX)) * 2.f * PI;
        const Engine::Vector3 vPlayer = pPlayerTr->GetPosition();
        const float fX = vPlayer.x + std::cos(fAngle) * rule.fRadius;
        const float fZ = vPlayer.z + std::sin(fAngle) * rule.fRadius;

        // Clamp to the floor extents so the spawn cell is always inside
        // the navigable voxel volume.
        const int iMax = GameWorldBuilder::kFloorSize - 1;
        const int cx = (std::max)(0, (std::min)(iMax,
            static_cast<int>(std::floor(fX))));
        const int cz = (std::max)(0, (std::min)(iMax,
            static_cast<int>(std::floor(fZ))));

        auto pEnemy = m_pScene->CreateGameObject<Enemy>("Enemy", pLayer);
        if (!pEnemy) return;

        // Variant selection: rule.enemy_id picks a specific row (> 0),
        // or round-robins through the database (≤ 0). The round-robin
        // index is per-rule so two parallel rules don't share an index.
        if (rule.iEnemyId > 0)
        {
            if (const EnemyDef* pDef = EnemyDatabase::GetInst().Get(rule.iEnemyId))
                pEnemy->ApplyDef(*pDef);
        }
        else
        {
            const auto& vecDefs = EnemyDatabase::GetInst().All();
            if (!vecDefs.empty())
            {
                const size_t idx = static_cast<size_t>(state.iSpawnIdx) % vecDefs.size();
                pEnemy->ApplyDef(vecDefs[idx]);
            }
        }
        ++state.iSpawnIdx;

        pEnemy->SetVoxelWorld(m_pWorld);
        pEnemy->SetFlowField(m_pFlowField.get());
        pEnemy->SetSpawnCell(cx, cz);
        pEnemy->SetTarget(pPlayer);
    }
}

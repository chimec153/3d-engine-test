#include "EnemySpawner.h"
#include "GameWorldBuilder.h"
#include "../Object/Enemy.h"
#include "../Object/EnemyDatabase.h"
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
        : m_pScene(pScene), m_pWorld(pWorld)
    {
    }

    void EnemySpawner::Tick(float fDeltaTime)
    {
        if (!m_pScene || !m_pWorld) return;

        const float fInterval = SpawnConfig::GetInst().GetEnemyInterval();
        m_fSpawnAcc += fDeltaTime;
        if (m_fSpawnAcc < fInterval) return;
        m_fSpawnAcc -= fInterval;

        auto pLayer = m_pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        auto pPlayer = pLayer->FindGameObject("player");
        if (!pPlayer) return;
        auto pPlayerTr = pPlayer->GetComponent<Engine::Transform>();
        if (!pPlayerTr) return;

        const float fRadius = SpawnConfig::GetInst().GetEnemyRadius();
        const float fAngle =
            (std::rand() / static_cast<float>(RAND_MAX)) * 2.f * PI;
        const Engine::Vector3 vPlayer = pPlayerTr->GetPosition();
        const float fX = vPlayer.x + std::cos(fAngle) * fRadius;
        const float fZ = vPlayer.z + std::sin(fAngle) * fRadius;

        // Clamp to the floor extents so the spawn cell is always inside
        // the navigable voxel volume. Floor size lives on the builder so
        // both sides agree without a separate constant.
        const int iMax = GameWorldBuilder::kFloorSize - 1;
        const int cx = (std::max)(0, (std::min)(iMax,
            static_cast<int>(std::floor(fX))));
        const int cz = (std::max)(0, (std::min)(iMax,
            static_cast<int>(std::floor(fZ))));

        auto pEnemy = m_pScene->CreateGameObject<Enemy>("Enemy", pLayer);
        if (!pEnemy) return;

        // Round-robin through EnemyDatabase rows so every variant shows
        // up in the demo. ApplyDef writes HP / speed / attack and binds
        // the matching mesh + material colour. If the DB is empty (CSV
        // missing) the enemy keeps its compiled-in defaults.
        const auto& vecDefs = EnemyDatabase::GetInst().All();
        if (!vecDefs.empty())
        {
            const size_t idx = static_cast<size_t>(m_iSpawnIdx) % vecDefs.size();
            pEnemy->ApplyDef(vecDefs[idx]);
        }
        ++m_iSpawnIdx;

        pEnemy->SetVoxelWorld(m_pWorld);
        pEnemy->SetSpawnCell(cx, cz);
        pEnemy->SetTarget(pPlayer);
    }
}

#pragma once
#include "../WeaponData.h"
#include "../Enemy.h"
#include "Bindable/Transform.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Types.h"
#include <memory>
#include <vector>
#include <cfloat>
#include <cstdlib>

namespace Client
{
    // Shared enemy-target picker for the auto-targeting movers (Aimed / Homing).
    // Extracted from the (previously duplicated) nearest-enemy scan those two
    // ran inline -- the TODO comment in AimedMovement asked for exactly this
    // once a third targeting path appeared, and AimMode is that third path.
    //
    // Scans active "Enemy" objects on DEFAULT_LAYER of the active scene (the
    // bullet's scene while it lives, so no scene plumbing is needed) and
    // returns the one the mode selects, or nullptr when no enemy exists.
    // Header-only inline so both movers share it without a new translation
    // unit; callers read the target position from the returned object's
    // Transform.
    inline std::shared_ptr<Engine::GameObject> FindTargetEnemy(
        const Engine::Vector3& from, AimMode eMode,
        const std::vector<Engine::GameObject*>* pExclude = nullptr)
    {
        auto pScene = Engine::SceneManager::GetInst()->GetScene();
        if (!pScene) return nullptr;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return nullptr;

        std::shared_ptr<Engine::GameObject> pBest;
        float fBestKey = FLT_MAX;   // nearest dist^2, or lowest HP
        std::vector<std::shared_ptr<Engine::GameObject>> vCandidates;  // Random only

        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
            if (pExclude)   // Chain: skip enemies already struck this flight
            {
                bool bSkip = false;
                for (const Engine::GameObject* x : *pExclude)
                    if (x == p.get()) { bSkip = true; break; }
                if (bSkip) continue;
            }

            switch (eMode)
            {
            case AimMode::LowestHP:
            {
                auto* pEnemy = dynamic_cast<Enemy*>(p.get());
                if (!pEnemy) continue;
                const float hp = static_cast<float>(pEnemy->GetHP());
                if (hp < fBestKey) { fBestKey = hp; pBest = p; }
                break;
            }
            case AimMode::Random:
                vCandidates.push_back(p);
                break;
            case AimMode::Nearest:
            default:
            {
                auto pTr = p->GetComponent<Engine::Transform>();
                if (!pTr) continue;
                const Engine::Vector3 e = pTr->GetPosition();
                const float dx = e.x - from.x;
                const float dz = e.z - from.z;
                const float d2 = dx * dx + dz * dz;
                if (d2 < fBestKey) { fBestKey = d2; pBest = p; }
                break;
            }
            }
        }

        // Random picks uniformly from everything gathered above. std::rand is
        // the codebase's inline PRNG of record -- <random> is banned here
        // (the global `epsilon` macro mangles its headers).
        if (eMode == AimMode::Random && !vCandidates.empty())
            pBest = vCandidates[static_cast<size_t>(std::rand()) % vCandidates.size()];

        return pBest;
    }
}

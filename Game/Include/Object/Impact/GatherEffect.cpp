#include "GatherEffect.h"
#include "../Enemy.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include "Bindable/Transform.h"
#include "Types.h"

namespace Client
{
    void GatherEffect::OnImpact(const ImpactContext& ctx)
    {
        // Same active-scene enemy enumeration as HomingMovement: the bullet's
        // scene is the active scene while it lives, so SceneManager hands us
        // the right layer without threading a scene pointer through.
        auto pScene = Engine::SceneManager::GetInst()->GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const float fRadiusSq = m_fRadius * m_fRadius;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
            auto* pEnemy = dynamic_cast<Enemy*>(p.get());
            if (!pEnemy) continue;
            auto pTr = pEnemy->GetComponent<Engine::Transform>();
            if (!pTr) continue;

            // AoE range test (flattened to XZ). The pull itself is delegated to
            // the enemy, which scales the impulse by distance so the slide ends
            // on the impact point instead of a fixed distance past/short of it.
            Engine::Vector3 vToImpact = ctx.vImpactPos - pTr->GetPosition();
            vToImpact.y = 0.f;
            const float d2 = vToImpact.LengthSq();
            if (d2 > fRadiusSq || d2 < 1e-6f) continue;   // out of range / on top

            pEnemy->PullToward(ctx.vImpactPos, m_fPullFraction);
        }
    }
}

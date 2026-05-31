#include "KnockbackEffect.h"
#include "../Enemy.h"
#include "Bindable/Transform.h"

namespace Client
{
    void KnockbackEffect::OnImpact(const ImpactContext& ctx)
    {
        auto* pEnemy = dynamic_cast<Enemy*>(ctx.pHitEnemy);
        if (!pEnemy) return;

        auto pTr = pEnemy->GetComponent<Engine::Transform>();
        if (!pTr) return;

        // Push direction = enemy position minus impact point, flattened to XZ.
        Engine::Vector3 vDir = pTr->GetPosition() - ctx.vImpactPos;
        vDir.y = 0.f;
        if (vDir.LengthSq() < 1e-6f) return;   // dead-centre hit: no clear dir
        vDir.Normalize();

        pEnemy->ApplyImpulse(vDir * m_fStrength);
    }
}

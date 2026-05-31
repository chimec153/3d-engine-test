#include "BurnEffect.h"
#include "../Enemy.h"

namespace Client
{
    void BurnEffect::OnImpact(const ImpactContext& ctx)
    {
        // dynamic_cast is fine at current hit volume; profile if hits scale up.
        if (auto* pEnemy = dynamic_cast<Enemy*>(ctx.pHitEnemy))
            pEnemy->ApplyBurn(m_iDmgPerTick, m_fDuration);
    }
}

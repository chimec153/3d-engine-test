#include "SlowEffect.h"
#include "../Enemy.h"

namespace Client
{
    void SlowEffect::OnImpact(const ImpactContext& ctx)
    {
        // dynamic_cast is fine at current hit volume; profile if hits scale up.
        if (auto* pEnemy = dynamic_cast<Enemy*>(ctx.pHitEnemy))
            pEnemy->ApplySlow(m_fFactor, m_fDuration);
    }
}

#include "ImpactEffectFactory.h"
#include "KnockbackEffect.h"
#include "GatherEffect.h"
#include "BurnEffect.h"
#include "SlowEffect.h"

namespace Client
{
    std::vector<std::unique_ptr<IImpactEffect>> MakeImpactEffects(const WeaponDef& def)
    {
        std::vector<std::unique_ptr<IImpactEffect>> out;
        if (def.uImpactMask & Impact_Knockback)
            out.push_back(std::make_unique<KnockbackEffect>(def.fKnockback));
        if (def.uImpactMask & Impact_Gather)
            out.push_back(std::make_unique<GatherEffect>(def.fGatherPull, def.fGatherRadius));
        if (def.uImpactMask & Impact_Burn)
            out.push_back(std::make_unique<BurnEffect>(def.iBurnDamage, def.fBurnDuration));
        if (def.uImpactMask & Impact_Slow)
            out.push_back(std::make_unique<SlowEffect>(def.fSlowFactor, def.fSlowDuration));
        return out;
    }
}

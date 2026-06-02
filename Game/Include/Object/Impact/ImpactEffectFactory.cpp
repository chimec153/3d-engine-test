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

    std::vector<std::unique_ptr<IImpactEffect>> MakeTowerImpactEffects(
        unsigned int uMask, float fP0, float fP1)
    {
        // One effect per tower in practice (one bit set), but a mask is handled
        // uniformly. fP0/fP1 carry that effect's params (see header / towers.csv).
        std::vector<std::unique_ptr<IImpactEffect>> out;
        if (uMask & Impact_Knockback)
            out.push_back(std::make_unique<KnockbackEffect>(fP0));
        if (uMask & Impact_Gather)
            out.push_back(std::make_unique<GatherEffect>(fP0, fP1));
        if (uMask & Impact_Burn)
            out.push_back(std::make_unique<BurnEffect>(static_cast<int>(fP0), fP1));
        if (uMask & Impact_Slow)
            out.push_back(std::make_unique<SlowEffect>(fP0, fP1));
        return out;
    }
}

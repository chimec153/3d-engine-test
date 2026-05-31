#pragma once
#include "IImpactEffect.h"
#include "../WeaponData.h"
#include <memory>
#include <vector>

namespace Client
{
    // Build the impact-effect strategies for a weapon from its module mask +
    // parameters. Damage is the baseline (applied by Enemy on hit), so only
    // Knockback / Gather produce strategies here. A weapon with neither set
    // gets an empty vector. Mirrors MakeBulletMovement.
    std::vector<std::unique_ptr<IImpactEffect>> MakeImpactEffects(const WeaponDef& def);
}

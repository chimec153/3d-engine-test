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

    // Build a tower's intrinsic impact effect(s) from a bare ImpactModule mask
    // + two generic params (fP0/fP1, meaning per effect — see towers.csv:
    // Slow=factor,duration; Gather=pull,radius; Burn=damage,duration;
    // Knockback=force,-). A Tower layers these onto the bullets it fires via
    // Bullet::AddImpactEffect. Mirrors MakeImpactEffects but mask-driven rather
    // than WeaponDef-driven. Impact_None / unset bits yield an empty vector.
    std::vector<std::unique_ptr<IImpactEffect>> MakeTowerImpactEffects(
        unsigned int uMask, float fP0, float fP1);
}

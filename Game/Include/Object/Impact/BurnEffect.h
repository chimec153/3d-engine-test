#pragma once
#include "IImpactEffect.h"

namespace Client
{
    // Applies a damage-over-time burn status to the struck enemy. The enemy
    // carries the timer and ticks the DoT itself (Enemy::ApplyBurn / Update);
    // re-hitting a burning enemy refreshes it (longer duration, higher tick).
    class BurnEffect : public IImpactEffect
    {
    public:
        BurnEffect(int iDmgPerTick, float fDuration)
            : m_iDmgPerTick(iDmgPerTick), m_fDuration(fDuration) {}
        void OnImpact(const ImpactContext& ctx) override;

    private:
        int   m_iDmgPerTick;
        float m_fDuration;
    };
}

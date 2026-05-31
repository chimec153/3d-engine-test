#pragma once
#include "IImpactEffect.h"

namespace Client
{
    // Slows the struck enemy's movement for a duration. The enemy carries the
    // timer and scales its own chase speed (Enemy::ApplySlow / Update);
    // re-hitting refreshes it (longer duration, stronger slow).
    class SlowEffect : public IImpactEffect
    {
    public:
        SlowEffect(float fFactor, float fDuration)
            : m_fFactor(fFactor), m_fDuration(fDuration) {}
        void OnImpact(const ImpactContext& ctx) override;

    private:
        float m_fFactor;    // 0..1 speed multiplier (lower = slower)
        float m_fDuration;
    };
}

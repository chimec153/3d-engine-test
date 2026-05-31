#pragma once
#include "IImpactEffect.h"

namespace Client
{
    // Shoves the struck enemy directly away from the impact point with a
    // fixed-magnitude impulse (world units/sec). The enemy decays the impulse
    // over the next frames (Enemy::Update), producing a brief knockback slide.
    class KnockbackEffect : public IImpactEffect
    {
    public:
        explicit KnockbackEffect(float fStrength) : m_fStrength(fStrength) {}
        void OnImpact(const ImpactContext& ctx) override;

    private:
        float m_fStrength;
    };
}

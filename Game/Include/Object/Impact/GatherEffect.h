#pragma once
#include "IImpactEffect.h"

namespace Client
{
    // Pulls every active enemy within a radius of the impact point toward it.
    // Each enemy is pulled fPullFraction of the way to the centre (1 = lands
    // exactly on it, clamped) via a decaying impulse, so a crowd converges on
    // the hit spot for AoE follow-up — no overshoot regardless of distance.
    class GatherEffect : public IImpactEffect
    {
    public:
        GatherEffect(float fPullFraction, float fRadius)
            : m_fPullFraction(fPullFraction), m_fRadius(fRadius) {}
        void OnImpact(const ImpactContext& ctx) override;

    private:
        float m_fPullFraction;   // 0..1 (clamped in Enemy::PullToward); 1 = full
        float m_fRadius;
    };
}

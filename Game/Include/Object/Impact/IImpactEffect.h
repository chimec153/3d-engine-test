#pragma once
#include "Vector3.h"

namespace Engine
{
    class GameObject;
}

namespace Client
{
    // Context handed to an impact effect when a bullet strikes an enemy.
    // The bullet fills it in OnBeginCollision: the world-space hit point and
    // the directly-struck enemy. Area effects (Gather) enumerate the scene
    // themselves; they only need the impact point.
    struct ImpactContext
    {
        Engine::Vector3     vImpactPos;            // world-space hit point
        Engine::GameObject* pHitEnemy = nullptr;   // the directly struck enemy
    };

    // Strategy interface for a weapon's on-impact effect. One concrete
    // strategy per ImpactModule bit (Knockback, Gather, ...). A Bullet owns a
    // vector of these and invokes each in OnBeginCollision, so a single weapon
    // can stack several effects. Damage is the baseline (applied by Enemy on
    // collision), so it produces no strategy here. Mirrors IBulletMovement.
    class IImpactEffect
    {
    public:
        virtual ~IImpactEffect() = default;

        // Apply the effect for one bullet->enemy hit.
        virtual void OnImpact(const ImpactContext& ctx) = 0;
    };
}

#pragma once
#include "IBulletMovement.h"
#include <memory>

namespace Engine { class GameObject; }

namespace Client
{
    // Guided / homing projectile. It locks onto one enemy chosen by the
    // weapon's AimMode and steers its heading toward it by a capped turn rate,
    // then advances forward like StraightMovement. The target is HELD (weak_ptr)
    // and only re-acquired when it dies / despawns, so LowestHP and Random
    // don't thrash the heading every frame. With no enemy available it flies
    // straight.
    class HomingMovement final : public IBulletMovement
    {
    public:
        virtual void Update(Engine::Transform& transform,
                            float fSpeed, float fDeltaTime) override;

        // Bullet forwards WeaponDef::eAimMode after Configure; selects which
        // enemy this missile locks onto. Defaults to Nearest (legacy).
        virtual void SetAimMode(AimMode e) override { m_eAimMode = e; }

    private:
        AimMode m_eAimMode = AimMode::Nearest;
        // Held target -- re-acquired only when it goes null / inactive.
        std::weak_ptr<Engine::GameObject> m_wpTarget;
    };
}

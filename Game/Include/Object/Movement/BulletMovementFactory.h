#pragma once
#include "IBulletMovement.h"
#include "../WeaponData.h"
#include <memory>

namespace Engine
{
    class Transform;
}

namespace Client
{
    // Maps WeaponDef::eMovement → concrete strategy. Orbital is the only
    // one that needs the owner; other types ignore the parameter.
    std::unique_ptr<IBulletMovement> MakeBulletMovement(
        MovementType eType,
        std::weak_ptr<Engine::Transform> pOwner);
}

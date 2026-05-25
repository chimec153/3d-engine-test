#include "BulletMovementFactory.h"
#include "StraightMovement.h"
#include "SpiralMovement.h"
#include "FixedMovement.h"
#include "OrbitalMovement.h"

namespace Client
{
    std::unique_ptr<IBulletMovement> MakeBulletMovement(
        MovementType eType,
        std::weak_ptr<Engine::Transform> pOwner)
    {
        switch (eType)
        {
        case MovementType::Spiral:  return std::make_unique<SpiralMovement>();
        case MovementType::Fixed:   return std::make_unique<FixedMovement>();
        case MovementType::Orbital: return std::make_unique<OrbitalMovement>(std::move(pOwner));
        case MovementType::Straight:
        default:                    return std::make_unique<StraightMovement>();
        }
    }
}

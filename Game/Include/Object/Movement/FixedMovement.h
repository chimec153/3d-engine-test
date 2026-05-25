#include "IBulletMovement.h"

namespace Client
{
    // No-op movement — bullet sits where it spawned until the lifetime
    // guard kills it (mines, cursor-shots).
    class FixedMovement final : public IBulletMovement
    {
    public:
        virtual void Update(Engine::Transform& /*transform*/,
                            float /*fSpeed*/, float /*fDeltaTime*/) override {}
    };
}

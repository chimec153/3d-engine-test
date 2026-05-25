#include "StraightMovement.h"
#include "Bindable/Transform.h"

namespace Client
{
    void StraightMovement::Update(Engine::Transform& transform,
                                  float fSpeed, float fDeltaTime)
    {
        const float fDist = fDeltaTime * fSpeed;
        transform.AddPosition(transform.GetAxis(Engine::AXIS_TYPE::Y) * fDist);
    }
}

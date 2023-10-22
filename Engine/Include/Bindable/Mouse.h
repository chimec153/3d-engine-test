#pragma once
#include "Drawable.h"

namespace Engine
{
    class ENGINE_DLL Mouse :
        public Drawable
    {
        friend class Scene;

    public:
        Mouse();
        virtual ~Mouse() override = default;

    private:
        std::shared_ptr<class ColliderLine>    m_pLineCollider;

    public:
        virtual void Update(float fDeltaTime) override;
    };

}
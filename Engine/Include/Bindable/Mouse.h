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
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
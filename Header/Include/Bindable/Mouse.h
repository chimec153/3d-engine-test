#pragma once
#include "../Component/Component.h"

namespace Engine
{
    // Phase B.6 — Mouse migrated from Drawable to Component. Mouse owns a
    // ColliderLine for picking/casting in screen space; doesn't render.
    // Holds its own Transform so that picking-related world position can
    // be queried via GetTransform()->GetPosition().
    class ENGINE_DLL Mouse :
        public Component
    {
        friend class Scene;

    public:
        Mouse();
        virtual ~Mouse() override = default;

    private:
        std::shared_ptr<class Transform> m_pTransform;
        std::shared_ptr<class ColliderLine>    m_pLineCollider;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;
        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
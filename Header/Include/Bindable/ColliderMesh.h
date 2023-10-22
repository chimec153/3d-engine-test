#pragma once
#include "Collider.h"

namespace Engine
{
    class ENGINE_DLL ColliderMesh :
        public Collider
    {
    public:
        ColliderMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex);
        ColliderMesh();
        ColliderMesh(const ColliderMesh& mesh);
        virtual ~ColliderMesh() override = default;

    private:
        std::shared_ptr<MESHCOLLIDERINFO> m_pInfo;

    public:
        const PMESHCOLLIDERINFO GetInfo()   const;
        void SetInfo(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex);

    private:
        virtual void Collision(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime);
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
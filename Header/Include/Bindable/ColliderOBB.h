#pragma once
#include "Collider.h"
namespace Engine
{
    class ENGINE_DLL ColliderOBB :
        public Collider
    {
    public:
        ColliderOBB();
        ColliderOBB(const ColliderOBB& collider);
        virtual ~ColliderOBB() override = default;

    private:
        OBBINFO m_tInfo;
        Vector3 m_vOffset;
        Vector3 m_vScaleOffset;
        Vector3 m_vAxisOffset;
#ifdef _DEBUG
        std::shared_ptr<Material> m_pDebugMaterial;
        std::shared_ptr<Transform> m_pDebugTransform;
#endif

    public:
        void SetAxis(AXIS_TYPE eType, const Vector3& vAxis);
        void SetCenter(const Vector3& vCenter);
        void SetOffset(const Vector3& vOffset);
        void SetScaleOffset(const Vector3& vScale);
        void SetAxisOffset(const Vector3& vOffset);
        const OBBINFO& GetInfo()    const;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual bool Collision(Collider* pCollider, float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;

    public:
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
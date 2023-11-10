#pragma once
#include "Collider.h"
namespace Engine
{
#ifdef _DEBUG
    template <typename T>
    class ConstantBuffer;
    template <typename T>
    class ConstantBuffer;
#endif
    class ENGINE_DLL ColliderSphere :
        public Collider
    {
    public:
        ColliderSphere();
        ColliderSphere(const ColliderSphere& collider);
    public:
        virtual ~ColliderSphere() override = default;

    private:
        Vector3 m_vOffset;
        SPHERECOLLIDERINFO  m_tInfo;
#ifdef _DEBUG
        std::shared_ptr<class ConstantBuffer<COLOR>>    m_pDebugPSConst;
#endif

    public:
        const SPHERECOLLIDERINFO& GetInfo() const;
        void SetRadius(float fRadius);
        void SetOffset(const Vector3& vOffset);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime) override;
        virtual std::shared_ptr<Bindable> Clone() override;
        virtual void PreDraw(float) override;
        virtual void Bind() override;
    };

}
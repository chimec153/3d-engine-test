#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Cloth :
        public Drawable
    {
    public:
        Cloth(int iWidth, int iHeight, float fSpring, float fSpringShear, float fSpringDistance, float fDamper, float fDamperShear, float fDamperDistance, float fDistance, float fMass);
        virtual ~Cloth() override = default;

    private:
        std::vector<VertexStandard>    m_vecPosition;
        std::vector<VertexStandard>    m_vecPrevPosition;
        std::vector<Vector3>    m_vecVelocity;
        std::vector<Vector3>    m_vecForce;
        int m_iWidth;
        int m_iHeight;
        float m_fSpring;
        float m_fSpringShear;
        float m_fSpringDistance;
        float m_fDamper;
        float m_fDamperShear;
        float m_fDamperDistance;
        float m_fDistance;
        float m_fMass;
        bool m_bSwitch;
        float m_fWind;
        Vector3 m_vWind;
        std::vector<unsigned int> m_vecIndex;
        std::shared_ptr<Mesh>   m_pMesh;
        std::shared_ptr<class ColliderSphere>   m_pCollider;

    public:
        void SetWindHeavyness(float fWind);
        void SetWind(const Vector3& vWind);
        float GetWindHeavyness()    const;

    public:
        virtual void FixedUpdate(float fDeltaTime) override;

    private:
        const Vector3& GetSpringForce(int iSrcIndex, int iDestIndex, float fSpring, float fDist) const;
        void ApplySpringForce(int iSrcIndex, int iDestIndex, float fSpring, float fDist);
        void ApplyDampForce(int iSrcIndex, int iDestIndex, float fDamp);
        void UpdateForce();
        void UpdatePosition(float fDeltaTime);
        void CreateVertexAndIndex();
        std::vector<VertexStandard>& GetCurrentVertexs();
        std::vector<VertexStandard>& GetPrevVertexs();

    public:
        void CollisionStay(class Collider* pSrc, Collider* pDest, float fDeltaTime);
    };
}
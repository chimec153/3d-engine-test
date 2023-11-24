#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Sphere :
        public Drawable
    {
        friend class Scene;
    public:
        Sphere(int iRings, int iSector);
        Sphere(const Sphere& sphere);
        virtual ~Sphere() override;

    private:
        float m_fSpeed;
        Vector3 m_vDir;

    public:
        float GetSpeed()    const;
        const Vector3& GetDir() const;
        void SetSpeed(float fSpeed);
        void SetDir(const Vector3& vDir);

    public:
        virtual bool Init() override;
        virtual void Input(float) override;
        virtual void Update(float fDeltaTime) override;
        virtual void Bind();
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        void CollisionEnter(class Collider* pSrc, class Collider* pDest, float fDeltaTime);

    public:
        template <typename T>
        static void CreateSphereVertex(int iRings, int iSector, std::vector<T>& pData)
        {
            pData.clear();

            T tVertex = {};

            tVertex.pos.x = 0.f;
            tVertex.pos.y = 0.5f;
            tVertex.pos.z = 0.f;

            pData.push_back(tVertex);

            for (int i = 0; i < iRings; ++i)
            {
                for (int j = 0; j < iSector; ++j)
                {
                    T tVertex;

                    float fAngle = PI / 2 - PI / static_cast<float>(iRings + 1) * (i + 1);

                    float fAngle2 = 2 * PI / static_cast<float>(iSector) * j;

                    tVertex.pos.x = cosf(fAngle2) * cosf(fAngle) / 2.f;
                    tVertex.pos.y = sinf(fAngle) / 2.f;
                    tVertex.pos.z = sinf(fAngle2) * cosf(fAngle) / 2.f;

                    pData.push_back(tVertex);
                }
            }

            tVertex.pos.y = -0.5f;

            pData.push_back(tVertex);
        }
        template <typename T>
        static void GetSphereVertexTexcoord(int iRings, int iSector, std::vector<T>& pData)
        {
            for (size_t i = 0; i < pData.size(); ++i)
            {
                float fLength = sqrtf(pData[i].pos.x * pData[i].pos.x + pData[i].pos.y * pData[i].pos.y + pData[i].pos.z * pData[i].pos.z);

                pData[i].normal.x = pData[i].pos.x / fLength;
                pData[i].normal.y = pData[i].pos.y / fLength;
                pData[i].normal.z = pData[i].pos.z / fLength;
            }
        }



        static void CreateSphereIndex(int iRings, int iSectors, std::vector<unsigned int>& vecIndex);
    };

}
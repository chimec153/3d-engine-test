#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Cylinder :
        public Drawable
    {
        friend class Scene;

    public:
        Cylinder(int iCount = 8);
        Cylinder(const Cylinder& cylinder);
    public:
        virtual ~Cylinder() override = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        template <typename T>
        static std::vector<T> CreateCylinderVertex(int iCount)
        {
            std::vector<T> vecVertex(iCount * 2);

            for (int i = 0; i < iCount; ++i)
            {
                vecVertex[i].pos.x = cosf(2 * PI / static_cast<float>(iCount) * i) / 2.f;
                vecVertex[i].pos.y = 0.f;
                vecVertex[i].pos.z = sinf(2 * PI / static_cast<float>(iCount) * i) / 2.f;

                vecVertex[i + iCount].pos.x = cosf(2 * PI / static_cast<float>(iCount) * i) / 2.f;
                vecVertex[i + iCount].pos.y = 1.f;
                vecVertex[i + iCount].pos.z = sinf(2 * PI / static_cast<float>(iCount) * i) / 2.f;
            }

            return vecVertex;
        }

        static std::vector<unsigned int> CreateCylinderIndex(int iCount);
    };

}
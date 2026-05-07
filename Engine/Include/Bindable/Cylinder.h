#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    // Phase E5 — Cylinder migrated from Drawable to Component shell.
    // Currently dead at runtime. Static CreateCylinderVertex /
    // CreateCylinderIndex helpers are preserved.
    class ENGINE_DLL Cylinder :
        public Component
    {
        friend class Scene;

    public:
        Cylinder();
        Cylinder(int iCount);
        Cylinder(const Cylinder& cylinder);
    public:
        virtual ~Cylinder() override = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

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

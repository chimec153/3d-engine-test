#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    // Phase E5 — Cone migrated from Drawable to Component shell. Currently
    // dead at runtime. Static CreateConeVertex / CreateConeIndex helpers
    // are preserved for future GameObject + MeshRenderer setups.
    class ENGINE_DLL Cone :
        public Component
    {
        friend class Scene;

    public:
        Cone();
        Cone(int iBaseCount);
        Cone(const Cone& cone);
    public:
        virtual ~Cone() override = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    public:
        template <typename T>
        static void CreateConeVertex(int iBaseCount, std::vector<T>& vecVertex)
        {
            T tVertex = {};

            tVertex.pos.x = 0.f;
            tVertex.pos.y = 1.f;
            tVertex.pos.z = 0.f;

            tVertex.r = 1.f;
            tVertex.g = 1.f;
            tVertex.b = 1.f;
            tVertex.a = 1.f;

            vecVertex.push_back(tVertex);

            for (int i = 0; i < iBaseCount; ++i)
            {
                T tVertex = {};

                tVertex.pos.x = cosf(PI * 2.f / iBaseCount * i) / 2.f;
                tVertex.pos.y = 0.f;
                tVertex.pos.z = sinf(PI * 2.f / iBaseCount * i) / 2.f;

                tVertex.r = 1.f;
                tVertex.g = 1.f;
                tVertex.b = 1.f;
                tVertex.a = 1.f;

                vecVertex.push_back(tVertex);
            }
        }

        void CreateConeIndex(int iBaseCount, std::vector<unsigned int>& vecIndex);
    };

}

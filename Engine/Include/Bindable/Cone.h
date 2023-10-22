#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Cone :
        public Drawable
    {
        friend class Scene;

    public:
        Cone(int iBaseCount);
        Cone(const Cone& cone);
    public:
        virtual ~Cone() override = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual  std::shared_ptr<Bindable> Clone() override;

    public:
        template <typename T>
        static void CreateConeVertex(int iBaseCount, std::vector<T>& vecVertex)
        {
            T tVertex = {};

            tVertex.pos.x = 0.f;
            tVertex.pos.y = 1.f;
            tVertex.pos.z = 0.f;

            /* tVertex.r = 1.f;
             tVertex.g = 0.f;
             tVertex.b = 0.f;
             tVertex.a = 1.f;*/

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

                /*tVertex.r = 0.f;
                tVertex.g = 0.f;
                tVertex.b = 1.f;
                tVertex.a = 1.f;*/
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
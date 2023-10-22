#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Quad :
        public Drawable
    {
        friend class Scene;
    public:
        Quad(const TCHAR* pFileName);
        Quad(const Quad& quad);
    public:
        virtual ~Quad() = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        template <typename T>
        static std::vector<T> CreateQuadVertex()
        {
            std::vector<T> vecVertex(4);

            vecVertex[0].pos.x = 0.f;
            vecVertex[0].pos.y = 1.f;
            vecVertex[1].pos.x = 1.f;
            vecVertex[1].pos.y = 1.f;
            vecVertex[2].pos.x = 0.f;
            vecVertex[2].pos.y = 0.f;
            vecVertex[3].pos.x = 1.f;
            vecVertex[3].pos.y = 0.f;

            vecVertex[0].uv.x = 0.f;
            vecVertex[0].uv.y = 0.f;
            vecVertex[1].uv.x = 1.f;
            vecVertex[1].uv.y = 0.f;
            vecVertex[2].uv.x = 0.f;
            vecVertex[2].uv.y = 1.f;
            vecVertex[3].uv.x = 1.f;
            vecVertex[3].uv.y = 1.f;

            return vecVertex;
        }
    };

}
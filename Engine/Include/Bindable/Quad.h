#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    // Phase E5 — Quad migrated from Drawable to Component shell. Currently
    // dead at runtime (no live construction path). The static
    // CreateQuadVertex<T> helper is preserved as a utility for future
    // GameObject + MeshRenderer setups that want a unit quad.
    class ENGINE_DLL Quad :
        public Component
    {
        friend class Scene;
    public:
        Quad();
        Quad(const TCHAR* pFileName);
        Quad(const Quad& quad);
    public:
        virtual ~Quad() = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

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

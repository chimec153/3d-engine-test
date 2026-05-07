#pragma once
#include "Component\Component.h"
#include "Types.h"

namespace Engine
{
    class Mesh;
    class VertexShader;
    class PixelShader;
    class Topology;
    class InputLayout;
    class RasterizerState;
    class Material;
    class Transform;
}

namespace Client
{
    // Phase E5 — Trail migrated from Drawable to Component. Owns its own
    // Transform plus the GPU resource slots needed for the procedural
    // strip mesh. Self-registers with RenderManager via PreDraw.
    class Trail :
        public Engine::Component
    {
    public:
        Trail();
        Trail(int iCount);
        Trail(const Trail& other);
        virtual ~Trail() override = default;

    private:
        std::vector<Engine::VertexStandard> m_vecVertex;
        std::vector<unsigned int> m_vecIndex;
        std::shared_ptr<Engine::Mesh>            m_pMesh;
        std::shared_ptr<Engine::VertexShader>    m_pVS;
        std::shared_ptr<Engine::PixelShader>     m_pPS;
        std::shared_ptr<Engine::Topology>        m_pTopology;
        std::shared_ptr<Engine::InputLayout>     m_pInputLayout;
        std::shared_ptr<Engine::RasterizerState> m_pRasterizerState;
        std::shared_ptr<Engine::Material>        m_pMaterial;
        std::shared_ptr<Engine::Transform>       m_pTransform;
        Engine::RENDER_LAYER m_eRenderLayer;

    public:
        void SetAllPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom);
        void SetPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom);

        void SetRenderLayer(Engine::RENDER_LAYER eLayer) { m_eRenderLayer = eLayer; }
        Engine::RENDER_LAYER GetRenderLayer() const { return m_eRenderLayer; }

        // Override Component::GetTransform — Trail owns its own Transform.
        virtual std::shared_ptr<Engine::Transform> GetTransform() const override { return m_pTransform; }

    public:
        virtual bool Init() override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Self-contained bind+draw called from RenderManager's blur pass.
        void Bind();
    };

}

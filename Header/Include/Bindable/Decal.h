#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    // Phase E5 — Decal migrated from Drawable to Component. Owns its own
    // Transform (the decal's projection volume), GPU resource slots
    // (Mesh/VS/PS/IL/Topology/Texture/Material), and the per-decal CB.
    // Render pipeline integration: PreDraw self-registers with
    // RenderManager; RenderDecal pass iterates m_DecalList and calls
    // Decal::Bind, which is a self-contained bind+draw.
    class ENGINE_DLL Decal :
        public Component
    {
    public:
        Decal();
        Decal(const Decal& decal);
        virtual ~Decal() override = default;
    private:
        DECALCBUFFER m_tCBuffer;
        std::shared_ptr<ConstantBuffer<DECALCBUFFER>>   m_pCBuffer;
        bool m_bFadeStart;

        std::shared_ptr<class Transform>     m_pTransform;
        std::shared_ptr<class Mesh>          m_pMesh;
        std::shared_ptr<class VertexShader>  m_pVS;
        std::shared_ptr<class PixelShader>   m_pPS;
        std::shared_ptr<class Topology>      m_pTopology;
        std::shared_ptr<class InputLayout>   m_pInputLayout;
        std::shared_ptr<class Texture>       m_pTexture;
        std::shared_ptr<class Material>      m_pMaterial;
        RENDER_LAYER m_eRenderLayer;

    public:
        void SetMaxFadeTime(float fMax);
        void SetFadeStartTime(float fStart);
        void StartFade();

        // Slots (mirrors MeshRendererComponent pattern).
        void SetMesh(const std::shared_ptr<class Mesh>& p);
        void SetVertexShader(const std::shared_ptr<class VertexShader>& p);
        void SetPixelShader(const std::shared_ptr<class PixelShader>& p);
        void SetTopology(const std::shared_ptr<class Topology>& p);
        void SetInputLayout(const std::shared_ptr<class InputLayout>& p);
        void SetTexture(const std::shared_ptr<class Texture>& p);
        void SetMaterial(const std::shared_ptr<class Material>& p);

        std::shared_ptr<class Mesh>     GetMesh()     const { return m_pMesh; }
        std::shared_ptr<class Texture>  GetTexture()  const { return m_pTexture; }
        std::shared_ptr<class Material> GetMaterial() const { return m_pMaterial; }

        // Override of Component::GetTransform — Decal owns its own
        // Transform (the decal projection volume).
        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

        void SetRenderLayer(RENDER_LAYER eLayer) { m_eRenderLayer = eLayer; }
        RENDER_LAYER GetRenderLayer() const { return m_eRenderLayer; }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Self-contained bind+draw called from RenderManager::RenderDecal.
        void Bind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}

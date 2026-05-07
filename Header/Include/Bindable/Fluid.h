#pragma once
#include "../Component/Component.h"
#define FLUID_BUFFER_COUNT 3
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    // Phase E5 — Fluid migrated from Drawable to Component. Currently
    // unused at runtime (no live construction path); kept as a usable
    // shell for future GameObject-based fluid simulation. The compute
    // pipeline (StructuredBuffers + CS + CB) is preserved; GPU rendering
    // resources that used to live on the Drawable child list belong on
    // a paired MeshRendererComponent in future setups.
    class ENGINE_DLL Fluid :
        public Component
    {
    public:
        Fluid();
        Fluid(int n, int m, float d, float mu, float c, float t = FIXED_UPDATE_TIME);//c^2 = T / rho
        virtual ~Fluid() override = default;
    private:
        std::shared_ptr<class StructuredBuffer> m_pBuffer[FLUID_BUFFER_COUNT];
        int    m_iCurrentBuffer;
        std::shared_ptr<class ComputeShader>    m_pCS;
        std::shared_ptr<ConstantBuffer<FLUIDCBUFFER>> m_pCBuffer;
        std::shared_ptr<class Mesh>             m_pMesh;
        FLUIDCBUFFER m_tCBuffer;
        int m_iHeight;

    public:
        void CreateVertexBufferAndIndexBuffer(int n, int m);

    public:
        virtual void Input(float fDeltaTime) override;
        virtual void FixedUpdate(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Called explicitly by an owning render pass that wants to feed the
        // fluid's current displacement buffer into the vertex stage.
        void Bind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    public:
        void Ready();
    };
}

#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    // Phase E5 — Particle migrated from Drawable to Component. Owns its
    // own Transform (the particle emitter's anchor) and the GPU resources
    // that the simulation/render path depends on. Self-registers with
    // RenderManager via PreDraw; rendered alongside m_RenderList[layer]
    // by the alpha / blur passes.
    class ENGINE_DLL Particle :
        public Component
    {
    public:
        Particle();
        Particle(int iMaxCount);
        Particle(const Particle& other);
        virtual ~Particle() override = default;

    private:
        std::shared_ptr<class VertexShader>   m_pVS;
        std::shared_ptr<class GeometryShader>   m_pGS;
        std::shared_ptr<class PixelShader>   m_pPS;
        std::shared_ptr<class ComputeShader>  m_pCS;
        PARTICLECBUFFER m_tCBuffer;
        std::shared_ptr<class StructuredBuffer> m_pBuffer;
        std::shared_ptr<class StructuredBuffer> m_pSystemBuffer;
        std::shared_ptr<ConstantBuffer<PARTICLECBUFFER>>   m_pParticleCBuffer;
        float   m_fElapsedTime;
        float   m_fEmitMaxTime;
        std::shared_ptr<class BlendState>   m_pBlendState;
        std::shared_ptr<class Texture>      m_pTexture;
        std::shared_ptr<class Transform>    m_pTransform;
        bool m_bStopEmit;
        int m_iPrevCreateGroupOffset;
        int m_iEmitCount;
        RENDER_LAYER m_eRenderLayer;
#ifdef _DEBUG
        std::vector<bool> m_vecPrevAlive;
#endif

    public:
        void SetStartColor(const Vector4& vColor);
        void SetEndColor(const Vector4& vColor);
        void SetVelocity(const Vector3& vVelocity);
        void SetAccelaration(const Vector3& vAccel);
        void SetMaxLifeTime(float fMaxTime);
        void SetMaxParticleCount(int iMaxCount);
        void SetMaxCreatePosition(const Vector3& vEnd);
        void SetMinCreatePosition(const Vector3& vStart);
        void SetEmitTime(float fEmitTime);
        void SetStartSize(const Vector2& vSize);
        void SetEndSize(const Vector2& vSize);
        void SetMaxFrame(int iFrame);
        void SetFrameWidth(int iWidth);
        void SetFrameHeight(int iHeight);
        void SetMaxVelocity(const Vector3& vMaxVelocity);
        void StopEmit();
        void ResumeEmit();
        void AddEmitCount(int iCount);

        void SetTexture(const std::shared_ptr<class Texture>& p) { m_pTexture = p; }
        std::shared_ptr<class Texture> GetTexture() const { return m_pTexture; }

        void SetRenderLayer(RENDER_LAYER eLayer) { m_eRenderLayer = eLayer; }
        RENDER_LAYER GetRenderLayer() const { return m_eRenderLayer; }

        // Override Component::GetTransform — Particle owns its own
        // Transform (the emitter anchor).
        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual void PostUpdate(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Self-contained bind+draw called from RenderManager's alpha / blur
        // passes via the Component-side particle list.
        void Bind();

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}

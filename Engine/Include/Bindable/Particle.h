#pragma once
#include "Drawable.h"
namespace Engine
{
    template <typename T>
    class ConstantBuffer;

    template <typename T>
    class ConstantBuffer;

    class ENGINE_DLL Particle :
        public Drawable
    {
    public:
        Particle();
        Particle(int iMaxCount);
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
        bool m_bStopEmit;
        int m_iPrevCreateGroupOffset;
        int m_iEmitCount;
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

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    };
}
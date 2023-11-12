#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Fluid :
        public Drawable
    {
    public:
        Fluid(int n, int m, int d, float p, float mu, float c, float t = FIXED_UPDATE_TIME);//c^2 = T / rho
        virtual ~Fluid() override = default;
    private:
        std::shared_ptr<class StructuredBuffer> m_pBuffer[3];
        int    m_iCurrentBuffer;
        std::shared_ptr<class ComputeShader>    m_pCS;
        std::shared_ptr<class ConstantBuffer<FLUIDCBUFFER>> m_pCBuffer;
        FLUIDCBUFFER m_tCBuffer;
        int m_iHeight;

    public:
        virtual void FixedUpdate(float fDeltaTime) override;
    };
}
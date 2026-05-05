#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL Sampler :
        public Bindable
    {
        template <typename T>
        friend class BindableManager;
    public:
        Sampler(D3D11_FILTER filter, UINT iSlot = 0, D3D11_TEXTURE_ADDRESS_MODE eAddress = D3D11_TEXTURE_ADDRESS_WRAP, D3D11_COMPARISON_FUNC eFunc = D3D11_COMPARISON_NEVER);
        virtual ~Sampler() override = default;

    private:
        CPtr<ID3D11SamplerState> m_pState;
        UINT    m_iSlot;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

        // Sort-by-state cache — see VertexShader::ResetBoundCache.
        static void ResetBoundCache();

    private:
        static constexpr UINT kMaxSlots = 8;
        static ID3D11SamplerState* s_pBound[kMaxSlots];
    };

}
#pragma once
#include "Bindable.h"
#include "BindableManager.h"
namespace Engine
{
    class ENGINE_DLL RasterizerState :
        public Bindable
    {
        friend class BindableManager<RasterizerState>;
    public:
        RasterizerState(bool bDepthEnable, D3D11_CULL_MODE eCullMode, D3D11_FILL_MODE eFillmode, float fDepthBias = 0.f, float fSlopeScaledDepthBias = 0.f, bool bScissorEnable = false);
        ~RasterizerState() = default;

    private:
        CPtr<ID3D11RasterizerState> m_pState;
        CPtr<ID3D11RasterizerState> m_pPrevState;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
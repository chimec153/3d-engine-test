#pragma once
#include "Bindable.h"
#include "BindableManager.h"
namespace Engine
{
    class ENGINE_DLL BlendState :
        public Bindable
    {
        friend class BindableManager<BlendState>;

    public:
        BlendState(D3D11_BLEND srcBlend = D3D11_BLEND_SRC_ALPHA, D3D11_BLEND destBlend = D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP blendOp = D3D11_BLEND_OP_ADD,
            D3D11_BLEND srcBlendAlpha = D3D11_BLEND_ONE, D3D11_BLEND destBlendAlpha = D3D11_BLEND_ZERO, D3D11_BLEND_OP blendOpAlpha = D3D11_BLEND_OP_ADD);
        virtual ~BlendState() = default;

    private:
        CPtr<ID3D11BlendState>  m_pBlendState;
        CPtr<ID3D11BlendState>  m_pPrevBlendState;
        Vector4 m_vPrevColor;
        UINT   m_iPrevMask;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
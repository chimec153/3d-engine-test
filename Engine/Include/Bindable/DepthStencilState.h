#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL DepthStencilState :
        public Bindable
    {
        friend class BindableManager<DepthStencilState>;
    public:
        DepthStencilState(bool bDepthEnable, D3D11_DEPTH_WRITE_MASK eMask = D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_FUNC eFunc = D3D11_COMPARISON_LESS_EQUAL,
            bool bStencilEnable = false, UINT8 iStencilReadMask = 0, UINT8 iStencilWriteMask = 0, 
            D3D11_COMPARISON_FUNC eStencilFrontComparison = D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP eStencilFrontFailOp = D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP eStencilFrontDepthFailOp = D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP eStencilFrontPassOp = D3D11_STENCIL_OP_KEEP,
            D3D11_COMPARISON_FUNC eStencilBackComparison = D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP eStencilBackFailOp = D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP eStencilBackDepthFailOp = D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP eStencilBackPassOp = D3D11_STENCIL_OP_KEEP);
        ~DepthStencilState() = default;

    private:
        CPtr<ID3D11DepthStencilState>   m_pState;
        CPtr<ID3D11DepthStencilState>   m_pPrevState;
        UINT    m_iStencil;
        UINT    m_iPrevStencil;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
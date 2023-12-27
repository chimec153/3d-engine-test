#include "DepthStencilState.h"

namespace Engine
{
	DepthStencilState::DepthStencilState(bool bDepthEnable, D3D11_DEPTH_WRITE_MASK eMask, D3D11_COMPARISON_FUNC eFunc, 
		bool bStencilEnable, UINT8 iStencilReadMask, UINT8 iStencilWriteMask, 
		D3D11_COMPARISON_FUNC eStencilFrontComparison, D3D11_STENCIL_OP eStencilFrontFailOp,
		D3D11_STENCIL_OP eStencilFrontDepthFailOp, D3D11_STENCIL_OP eStencilFrontPassOp,
		D3D11_COMPARISON_FUNC eStencilBackComparison, D3D11_STENCIL_OP eStencilBackFailOp,
		D3D11_STENCIL_OP eStencilBackDepthFailOp, D3D11_STENCIL_OP eStencilBackPassOp) :
		Bindable()
		, m_pState(nullptr)
		, m_pPrevState(nullptr)
		, m_iStencil(0)
		, m_iPrevStencil(0)
	{
		SetBindableType(BINDABLE_TYPE::DEPTH_STENCIL_STATE);

		D3D11_DEPTH_STENCIL_DESC desc = {};

		desc.DepthEnable = bDepthEnable;
		desc.DepthWriteMask = eMask;
		desc.DepthFunc = eFunc;

		desc.StencilEnable = bStencilEnable;
		desc.StencilReadMask = iStencilReadMask;
		desc.StencilWriteMask = iStencilWriteMask;

		desc.FrontFace.StencilFunc = eStencilFrontComparison;
		desc.FrontFace.StencilFailOp = eStencilFrontFailOp;
		desc.FrontFace.StencilDepthFailOp = eStencilFrontDepthFailOp;
		desc.FrontFace.StencilPassOp = eStencilFrontPassOp;

		desc.BackFace.StencilFunc = eStencilBackComparison;
		desc.BackFace.StencilFailOp = eStencilBackFailOp;
		desc.BackFace.StencilDepthFailOp = eStencilBackDepthFailOp;
		desc.BackFace.StencilPassOp = eStencilBackPassOp;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateDepthStencilState(&desc, &m_pState)))
		{
			return;
		}
	}

	void DepthStencilState::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->OMGetDepthStencilState(&m_pPrevState, &m_iPrevStencil);

		Graphics::GetInst()->GetDeviceContext()->OMSetDepthStencilState(*m_pState, m_iStencil);
	}

	void DepthStencilState::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->OMSetDepthStencilState(*m_pPrevState, m_iPrevStencil);

		m_pPrevState = nullptr;
	}
	std::shared_ptr<Bindable> DepthStencilState::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}
}
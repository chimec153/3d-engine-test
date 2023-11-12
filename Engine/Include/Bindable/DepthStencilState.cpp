#include "DepthStencilState.h"

namespace Engine
{
	DepthStencilState::DepthStencilState(bool bDepthEnable, D3D11_DEPTH_WRITE_MASK eMask, D3D11_COMPARISON_FUNC eFunc) :
		Bindable()
		, m_pState(nullptr)
		, m_pPrevState(nullptr)
		, m_iStencil(0)
		, m_iPrevStencil(0)
	{
		D3D11_DEPTH_STENCIL_DESC desc = {};

		desc.StencilEnable = false;
		desc.DepthEnable = bDepthEnable;
		desc.DepthWriteMask = eMask;
		desc.DepthFunc = eFunc;

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
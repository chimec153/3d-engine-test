#include "RasterizerState.h"

namespace Engine
{
	RasterizerState::RasterizerState(bool bDepthEnable, D3D11_CULL_MODE eCullMode, D3D11_FILL_MODE eFillmode, float fDepthBias, float fSlopeScaledDepthBias) :
		Bindable()
	{
		SetBindableType(BINDABLE_TYPE::RASTERIZER_STATE);

		D3D11_RASTERIZER_DESC desc = {};

		desc.CullMode = eCullMode;
		desc.FillMode = eFillmode;
		desc.DepthClipEnable = bDepthEnable;
		desc.DepthBiasClamp = fDepthBias;
		desc.SlopeScaledDepthBias = fSlopeScaledDepthBias;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateRasterizerState(&desc, &m_pState)))
		{
			return;
		}
	}

	void RasterizerState::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->RSGetState(&m_pPrevState);

		Graphics::GetInst()->GetDeviceContext()->RSSetState(*m_pState);

	}

	void RasterizerState::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->RSSetState(*m_pPrevState);

		m_pPrevState = nullptr;
	}
	std::shared_ptr<Bindable> RasterizerState::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}
}
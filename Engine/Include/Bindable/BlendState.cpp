#include "BlendState.h"

namespace Engine
{
	BlendState::BlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, D3D11_BLEND_OP blendOp,
		D3D11_BLEND srcBlendAlpha, D3D11_BLEND destBlendAlpha, D3D11_BLEND_OP blendOpAlpha) :
		Bindable()
		, m_pBlendState(nullptr)
		, m_pPrevBlendState(nullptr)
		, m_vPrevColor()
		, m_iPrevMask(0)
	{
		SetBindableType(BINDABLE_TYPE::BLEND_STATE);

		D3D11_BLEND_DESC desc = {};

		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].BlendOp = blendOp;
		desc.RenderTarget[0].SrcBlend = srcBlend;
		desc.RenderTarget[0].DestBlend = destBlend;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		desc.RenderTarget[0].BlendOpAlpha = blendOpAlpha;
		desc.RenderTarget[0].SrcBlendAlpha = srcBlendAlpha;
		desc.RenderTarget[0].DestBlendAlpha = destBlendAlpha;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBlendState(&desc, &m_pBlendState)))
		{
			assert(false);
			return;
		}
	}

	BlendState::BlendState(bool bAlphaToCoverageEnable, bool bIndependentBlendEnable, const std::vector<D3D11_RENDER_TARGET_BLEND_DESC>& vecRenderTargetDesc)	:
		Bindable()
		, m_vPrevColor()
		, m_iPrevMask()
	{
		D3D11_BLEND_DESC tDesc = {};

		tDesc.AlphaToCoverageEnable = bAlphaToCoverageEnable;
		tDesc.IndependentBlendEnable = bIndependentBlendEnable;

		assert(vecRenderTargetDesc.size() <= 8);

		for (int i = 0; i < vecRenderTargetDesc.size(); ++i)
		{
			tDesc.RenderTarget[i] = vecRenderTargetDesc[i];
		}

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBlendState(&tDesc, &m_pBlendState)))
		{
			assert(false);
			return;
		}
	}

	void BlendState::Bind()
	{
		float color[] = { 1.f, 1.f, 1.f, 1.f };

		Graphics::GetInst()->GetDeviceContext()->OMGetBlendState(&m_pPrevBlendState, &m_vPrevColor.x, &m_iPrevMask);

		Graphics::GetInst()->GetDeviceContext()->OMSetBlendState(*m_pBlendState, color, 0xffffffff);
	}

	void BlendState::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->OMSetBlendState(*m_pPrevBlendState, &m_vPrevColor.x, m_iPrevMask);

		m_pPrevBlendState = nullptr;
	}
	std::shared_ptr<Bindable> BlendState::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}
}
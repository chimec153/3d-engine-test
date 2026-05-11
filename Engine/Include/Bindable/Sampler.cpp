#include "Sampler.h"

namespace Engine
{
	Sampler::Sampler(D3D11_FILTER filter, UINT iSlot, D3D11_TEXTURE_ADDRESS_MODE eAddress, D3D11_COMPARISON_FUNC eFunc) :
		Bindable()
		, m_pState(nullptr)
		, m_iSlot(iSlot)
	{
		D3D11_SAMPLER_DESC desc = {};

		desc.Filter = filter;
		desc.AddressU = eAddress;
		desc.AddressV = eAddress;
		desc.AddressW = eAddress;
		desc.ComparisonFunc = eFunc;
		desc.BorderColor[0] = 1.f;
		desc.BorderColor[1] = 1.f;
		desc.BorderColor[2] = 1.f;
		desc.BorderColor[3] = 1.f;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateSamplerState(&desc, &m_pState)))
		{
			assert(false);
			return;
		}
	}

	void Sampler::Update(float fDeltaTime)
	{
	}

	void Sampler::Bind()
	{
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		ID3D11SamplerState* mine = *m_pState;
		if (m_iSlot < BindCache::kSamplerSlots && cache.pBoundSamplers[m_iSlot] == mine)
			return;
		Graphics::GetInst()->GetDeviceContext()->VSSetSamplers(m_iSlot, 1, m_pState.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetSamplers(m_iSlot, 1, m_pState.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetSamplers(m_iSlot, 1, m_pState.GetAddressof());
		if (m_iSlot < BindCache::kSamplerSlots) cache.pBoundSamplers[m_iSlot] = mine;
	}
	std::shared_ptr<Bindable> Sampler::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}
}
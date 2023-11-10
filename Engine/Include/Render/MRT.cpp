#include "MRT.h"
#include "../Core/Window.h"

namespace Engine
{
	MRT::MRT(const std::vector<DXGI_FORMAT>& format, UINT iSlot,
		DXGI_FORMAT eDepthTextureFormat, DXGI_FORMAT eDSVFormat, DXGI_FORMAT eDepthSRVFormat) :
		m_pDSV(nullptr)
		, m_pPrevDSV(nullptr)
		, m_iSlot(iSlot)
	{
		CPtr<ID3D11Texture2D> pDepthTexture = nullptr;

		D3D11_TEXTURE2D_DESC tDepthTextureDesc = {};

		tDepthTextureDesc.Format = eDepthTextureFormat;
		tDepthTextureDesc.ArraySize = 1;
		tDepthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		tDepthTextureDesc.Width = Window::GetInst()->GetWidth();
		tDepthTextureDesc.Height = Window::GetInst()->GetHeight();
		tDepthTextureDesc.MipLevels = 1;
		tDepthTextureDesc.SampleDesc.Count = 1;
		tDepthTextureDesc.SampleDesc.Quality = 0;
		tDepthTextureDesc.Usage = D3D11_USAGE_DEFAULT;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateTexture2D(&tDepthTextureDesc, nullptr, &pDepthTexture)))
		{
			assert(false);
			return;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC tDSVDesc = {};

		tDSVDesc.Format = eDSVFormat;
		tDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		tDSVDesc.Texture2D.MipSlice = 0;
		tDSVDesc.Flags = 0;

		CPtr<ID3D11DepthStencilView> pDSV = nullptr;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateDepthStencilView(*pDepthTexture, &tDSVDesc, &m_pDSV)))
		{
			assert(false);
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC tSRVDesc = {};

		tSRVDesc.Format = eDepthSRVFormat;
		tSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		tSRVDesc.Texture2D.MipLevels = 1;
		tSRVDesc.Texture2D.MostDetailedMip = 0;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateShaderResourceView(*pDepthTexture, &tSRVDesc, &m_pDepthSRV)))
		{
			assert(false);
			return;
		}

		for (size_t i = 0; i < format.size(); ++i)
		{
			D3D11_TEXTURE2D_DESC textureDesc = {};

			textureDesc.Format = format[i];
			textureDesc.ArraySize = 1;
			textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			textureDesc.Width = Window::GetInst()->GetWidth();
			textureDesc.Height = Window::GetInst()->GetHeight();
			textureDesc.MipLevels = 0;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;

			CPtr<ID3D11Texture2D> pTexture = nullptr;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &pTexture)))
			{
				assert(false);
				continue;
			}

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};

			desc.Format = format[i];
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			CPtr<ID3D11RenderTargetView> pRTV = nullptr;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateRenderTargetView(*pTexture, &desc, &pRTV)))
			{
				assert(false);
				continue;
			}

			m_vecRTV.push_back(pRTV);

			D3D11_SHADER_RESOURCE_VIEW_DESC tTargetSRVDesc = {};

			tTargetSRVDesc.Format = format[i];
			tTargetSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			tTargetSRVDesc.Texture2D.MipLevels = 1;
			tTargetSRVDesc.Texture2D.MostDetailedMip = 0;

			CPtr<ID3D11ShaderResourceView> pSRV = nullptr;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateShaderResourceView(*pTexture, &tTargetSRVDesc, &pSRV)))
			{
				assert(false);
				continue;
			}

			m_vecSRV.push_back(pSRV);
		}
	}

	const std::vector<CPtr<ID3D11ShaderResourceView>>& MRT::GetSRVs() const
	{
		return m_vecSRV;
	}

	Engine::CPtr<ID3D11ShaderResourceView> MRT::GetDepthSRV() const
	{
		return m_pDepthSRV;
	}

	void MRT::Clear(D3D11_CLEAR_FLAG eClearFlag)
	{
		float color[] = { 0.f, 0.f, 0.f, 0.f };

		for (size_t i = 0; i < m_vecRTV.size(); ++i)
		{
			Graphics::GetInst()->GetDeviceContext()->ClearRenderTargetView(*m_vecRTV[i], color);
		}

		Graphics::GetInst()->GetDeviceContext()->ClearDepthStencilView(*m_pDSV, eClearFlag, 1.f, 0);
	}

	void MRT::SetTargets()
	{
		std::vector<ID3D11RenderTargetView*> vecRTV(8);

		Graphics::GetInst()->GetDeviceContext()->OMGetRenderTargets((UINT)vecRTV.size(), &vecRTV[0], m_pPrevDSV.GetAdressof());

		for (size_t i = 0; i < vecRTV.size(); ++i)
		{
			m_vecPrevRTV.push_back(vecRTV[i]);
		}

		vecRTV.clear();

		for (size_t i = 0; i < m_vecRTV.size(); ++i)
		{
			vecRTV.push_back(*m_vecRTV[i]);
		}

		if (vecRTV.size())
		{
			Graphics::GetInst()->GetDeviceContext()->OMSetRenderTargets((UINT)m_vecRTV.size(), &vecRTV[0], *m_pDSV);
		}
		else
		{
			Graphics::GetInst()->GetDeviceContext()->OMSetRenderTargets(0U, nullptr, *m_pDSV);
		}
	}

	void MRT::SetRenderTargets()
	{
		std::vector<ID3D11RenderTargetView*> vecRTV(8);

		Graphics::GetInst()->GetDeviceContext()->OMGetRenderTargets((UINT)vecRTV.size(), &vecRTV[0], m_pPrevDSV.GetAdressof());

		for (size_t i = 0; i < vecRTV.size(); ++i)
		{
			m_vecPrevRTV.push_back(vecRTV[i]);
		}

		vecRTV.clear();

		for (size_t i = 0; i < m_vecRTV.size(); ++i)
		{
			vecRTV.push_back(*m_vecRTV[i]);
		}

		if (vecRTV.size())
		{
			Graphics::GetInst()->GetDeviceContext()->OMSetRenderTargets((UINT)m_vecRTV.size(), &vecRTV[0], nullptr);
		}
	}

	void MRT::ResetTargets()
	{
		std::vector<ID3D11RenderTargetView*> vecRTV;

		for (size_t i = 0; i < m_vecPrevRTV.size(); ++i)
		{
			vecRTV.push_back(*m_vecPrevRTV[i]);

			if (vecRTV[i])
			{
				vecRTV[i]->Release();
			}
		}

		Graphics::GetInst()->GetDeviceContext()->OMSetRenderTargets((UINT)m_vecPrevRTV.size(), &vecRTV[0], *m_pPrevDSV);

		m_vecPrevRTV.clear();

		m_pPrevDSV = nullptr;
	}

	void MRT::SetSRV()
	{
		std::vector<ID3D11ShaderResourceView*> vecSRV = {};

		for (size_t i = 0; i < m_vecSRV.size(); ++i)
		{
			vecSRV.push_back(*m_vecSRV[i]);
		}

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->HSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->DSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
	}

	void MRT::SetSRV(int iIndex, UINT iSlot)
	{
		assert(iIndex >= 0 && iIndex < m_vecSRV.size());

		std::vector<ID3D11ShaderResourceView*> vecSRV = {};

		for (size_t i = 0; i < m_vecSRV.size(); ++i)
		{
			vecSRV.push_back(*m_vecSRV[i]);
		}

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
		Graphics::GetInst()->GetDeviceContext()->HSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
		Graphics::GetInst()->GetDeviceContext()->DSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(iSlot, 1, &vecSRV[iIndex]);
	}

	void MRT::ResetSRV()
	{
		std::vector<ID3D11ShaderResourceView*> vecSRV(m_vecSRV.size());

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->HSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->DSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, (UINT)m_vecSRV.size(), &vecSRV[0]);
	}

	void MRT::SetDepthSRV(UINT iSlot)
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->HSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->DSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(iSlot, 1, m_pDepthSRV.GetAdressof());
	}

	void MRT::ResetSRV(UINT iSlot)
	{
		ID3D11ShaderResourceView* pSRV = nullptr;

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->HSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->DSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->GSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(iSlot, 1, &pSRV);
	}
}
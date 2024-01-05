#include "Graphics.h"
#include "PathManager.h"
#include "../Bindable/Camera.h"
#include "../Bindable/Transform.h"
#include "../Bindable/PointLight.h"

namespace Engine
{
	Graphics* Graphics::m_pInst = nullptr;

	namespace dx = DirectX;

	Graphics::Graphics() :
		pDevice(nullptr)
		, pSwapChain(nullptr)
		, pDeviceContext(nullptr)
		, pRenderTargetView(nullptr)
		, pCamera()
	{
	}

	bool Graphics::Init(HWND hWnd, int iWidth, int iHeight)
	{
		DXGI_SWAP_CHAIN_DESC desc = {};

		desc.BufferDesc.Width = 0;
		desc.BufferDesc.Height = 0;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Denominator = 0;
		desc.BufferDesc.RefreshRate.Numerator = 0;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.OutputWindow = hWnd;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = true;
		desc.Flags = 0;

		UINT iFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
		iFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlags, nullptr, 0, D3D11_SDK_VERSION, &desc, &pSwapChain, &pDevice, nullptr, &pDeviceContext)))
		{
			return false;
		}

		CPtr<ID3D11Resource> pBackBuffer = nullptr;

		pSwapChain->GetBuffer(0, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&pBackBuffer));

		if (FAILED(pDevice->CreateRenderTargetView(*pBackBuffer, nullptr, &pRenderTargetView)))
		{
			return false;
		}

		D3D11_DEPTH_STENCIL_DESC tDepthStencilDesc = {};

		tDepthStencilDesc.DepthEnable = true;
		tDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		tDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		tDepthStencilDesc.StencilEnable = false;
		tDepthStencilDesc.StencilReadMask = 0xff;
		tDepthStencilDesc.StencilWriteMask = 0xff;

		CPtr<ID3D11DepthStencilState> pDepthStencilState;

		if (FAILED(pDevice->CreateDepthStencilState(&tDepthStencilDesc, &pDepthStencilState)))
		{
			return false;
		}

		pDeviceContext->OMSetDepthStencilState(*pDepthStencilState, 0);

		D3D11_TEXTURE2D_DESC tTextureDesc = {};

		tTextureDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		tTextureDesc.Width = iWidth;
		tTextureDesc.Height = iHeight;
		tTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		tTextureDesc.MipLevels = 1;
		tTextureDesc.SampleDesc.Count = 1;
		tTextureDesc.SampleDesc.Quality = 0;
		tTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		tTextureDesc.ArraySize = 1;

		CPtr<ID3D11Texture2D> pTexture;

		if (FAILED(pDevice->CreateTexture2D(&tTextureDesc, nullptr, &pTexture)))
		{
			return false;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC tDSVDesc = {};

		tDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
		tDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		if (FAILED(pDevice->CreateDepthStencilView(*pTexture, &tDSVDesc, &pDepthStencilView)))
		{
			return false;
		}

		pDeviceContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressof(), *pDepthStencilView);

		D3D11_VIEWPORT tViewPort = { 0.f, 0.f, (float)iWidth,(float)iHeight,0.f, 1.f };

		pDeviceContext->RSSetViewports(1, &tViewPort);

		return true;
	}

	void Graphics::EndScene()
	{
		pSwapChain->Present(0u, 0u);
	}

	void Graphics::Clear(float r, float g, float b)
	{
		float color[4] = { r,g,b,1.f };

		pDeviceContext->ClearRenderTargetView(*pRenderTargetView, color);
		pDeviceContext->ClearDepthStencilView(*pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.f, 0);
	}

	void Graphics::SetRenderTarget()
	{
		pDeviceContext->OMSetRenderTargets(1, pRenderTargetView.GetAddressof(), *pDepthStencilView);
	}

	void Graphics::Update(float fDeltaTime)
	{
	}

	void Graphics::PostUpdate(float fDeltaTime)
	{
	}

	std::shared_ptr<class Camera> Graphics::GetCamera(CAMERA_TYPE eType) const
	{
		return pCamera[static_cast<int>(eType)];
	}

	void Graphics::SetCamera(std::shared_ptr<class Camera> _pCamera, CAMERA_TYPE eType)
	{
		pCamera[static_cast<int>(eType)] = _pCamera;
	}

	void Graphics::SetLight(const std::shared_ptr<class PointLight>& _pLight)
	{
		pLight = _pLight;
	}

	ID3D11DeviceContext* Graphics::GetDeviceContext() const
	{
		return *pDeviceContext;
	}

	ID3D11Device* Graphics::GetDevice() const
	{
		return *pDevice;
	}

	const CPtr<ID3D11RenderTargetView>& Graphics::GetRTV() const
	{
		return pRenderTargetView;
	}

	const CPtr<ID3D11DepthStencilView>& Graphics::GetDSV() const
	{
		return pDepthStencilView;
	}

	const std::shared_ptr<PointLight>& Graphics::GetLight() const
	{
		return pLight;
	}
}
#pragma once

#include "Ptr.h"

namespace Engine
{
	// Phase E7 — centralized "what's currently bound to the device context"
	// cache. Replaces the per-Bindable-subclass static caches
	// (VertexShader::s_pBoundVS, PixelShader::s_pBoundPS, ..., Texture::s_pBound)
	// with one struct held by Graphics. Each Bindable's Bind() consults this
	// cache to skip redundant XSSet calls; RenderManager calls Reset() at
	// pass boundaries instead of 6 individual ResetBoundCache() invocations.
	struct ENGINE_DLL BindCache
	{
		static constexpr int kSamplerSlots = 8;
		static constexpr int kTextureSlots = 64;

		ID3D11VertexShader*       pBoundVS                  = nullptr;
		ID3D11PixelShader*        pBoundPS                  = nullptr;
		ID3D11InputLayout*        pBoundIL                  = nullptr;
		D3D_PRIMITIVE_TOPOLOGY    eBoundTopology            = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		ID3D11SamplerState*       pBoundSamplers[kSamplerSlots] = {};
		ID3D11ShaderResourceView* pBoundTextures[kTextureSlots] = {};

		void Reset()
		{
			pBoundVS = nullptr;
			pBoundPS = nullptr;
			pBoundIL = nullptr;
			eBoundTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			for (int i = 0; i < kSamplerSlots; ++i) pBoundSamplers[i] = nullptr;
			for (int i = 0; i < kTextureSlots; ++i) pBoundTextures[i] = nullptr;
		}
	};

	class ENGINE_DLL Graphics
	{
		friend class Bindable;
		friend class ImguiManager;

	public:
		Graphics();
		~Graphics() = default;

	private:
		static Graphics* m_pInst;

	public:
		static Graphics* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new Graphics;
			}

			return m_pInst;
		}

		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		CPtr<ID3D11Device> pDevice;
		CPtr<IDXGISwapChain> pSwapChain;
		CPtr<ID3D11DeviceContext> pDeviceContext;
		CPtr<ID3D11RenderTargetView> pRenderTargetView;
		CPtr<ID3D11DepthStencilView> pDepthStencilView;
		std::shared_ptr<class Camera> pCamera[static_cast<int>(CAMERA_TYPE::END)];
		std::shared_ptr<class PointLight> pLight;
		BindCache m_bindCache;

	public:
		bool Init(HWND hWnd, int iWidth, int iHeight);
		void EndScene();
		void Clear(float r, float g, float b);
		void SetRenderTarget();
		void Update(float fDeltaTime);
		void PostUpdate(float fDeltaTime);

	public:
		std::shared_ptr<class Camera> GetCamera(CAMERA_TYPE eType = CAMERA_TYPE::NORMAL)	const;
		void SetCamera(std::shared_ptr<class Camera> pCamera, CAMERA_TYPE eType = CAMERA_TYPE::NORMAL);
		void SetLight(const std::shared_ptr<class PointLight>& pLight);
		ID3D11DeviceContext* GetDeviceContext()	const;
		ID3D11Device* GetDevice()	const;
		const CPtr<ID3D11RenderTargetView>& GetRTV()	const;
		// Swap-chain access for D2D / DXGI interop. Text renders straight
		// to backbuffer 0 by wrapping its IDXGISurface in a D2D render
		// target — needs the swap chain to query that surface.
		const CPtr<IDXGISwapChain>& GetSwapChain() const { return pSwapChain; }
		const CPtr<ID3D11DepthStencilView>& GetDSV()	const;
		const std::shared_ptr<class PointLight>& GetLight()	const;

		BindCache& GetBindCache() { return m_bindCache; }
		void ResetBindCache() { m_bindCache.Reset(); }
	};

}
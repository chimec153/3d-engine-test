#pragma once

#include "Ptr.h"

namespace Engine
{
	class ENGINE_DLL Graphics
	{
		friend class Bindable;
		friend class Drawable;
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
		const CPtr<ID3D11DepthStencilView>& GetDSV()	const;
		const std::shared_ptr<class PointLight>& GetLight()	const;
	};

}
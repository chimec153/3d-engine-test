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
		Matrix m_matProject;
		Matrix m_matView;
		Matrix m_matViewProject;
		std::shared_ptr<class Camera> pCamera;
		std::shared_ptr<class Camera> pCamera2;
		std::shared_ptr<class PointLight> pLight;
		float m_fAngle;
		float m_fRatio;
		float m_fNear;

	public:
		bool Init(HWND hWnd, int iWidth, int iHeight);
		void EndScene();
		void Clear(float r, float g, float b);
		void SetRenderTarget();
		void Update(float fDeltaTime);

	public:
		const Matrix& GetProjectMatrix()	const;
		const Matrix& GetViewProject()	const;
		const Matrix& GetView()	const;
		std::shared_ptr<class Camera> GetCamera()	const;
		void SetCamera(const std::shared_ptr<class Camera>& pCamera);
		void SetCamera2(const std::shared_ptr<class Camera>& pCamera);
		void SetLight(const std::shared_ptr<class PointLight>& pLight);
		ID3D11DeviceContext* GetDeviceContext()	const;
		ID3D11Device* GetDevice()	const;
		const CPtr<ID3D11RenderTargetView>& GetRTV()	const;
		const CPtr<ID3D11DepthStencilView>& GetDSV()	const;
		const std::shared_ptr<class PointLight>& GetLight()	const;
		float GetAngle()	const;
		float GetRatio()	const;
		float GetNear()	const;
	};

}
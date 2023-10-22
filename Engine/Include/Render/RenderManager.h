#pragma once

#include "../Core/Ptr.h"

namespace Engine
{
	class ENGINE_DLL RenderManager
	{
	private:
		RenderManager();
		~RenderManager();

	private:
		static RenderManager* m_pInst;

	public:
		static RenderManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new RenderManager;
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
		std::list<class std::shared_ptr<class PointLight>>	m_LightList[static_cast<int>(LIGHT_TYPE::END)];
		std::list<class std::shared_ptr<class Drawable>>	m_RenderList[2];
		std::list<class std::shared_ptr<class Drawable>>	m_ShadowList;
		std::unordered_map<size_t, class std::shared_ptr<class RenderInstancing>>	m_mapInstance[2];
		std::unordered_map<size_t, class std::shared_ptr<class RenderInstancing>>	m_mapShadowInstance;

	private:
		std::shared_ptr<class MRT> pMRT;
#ifdef _DEBUG
		std::shared_ptr<VertexShader> pVertexShader;
		std::shared_ptr<PixelShader> pPixelShader;
#endif
		std::shared_ptr<VertexShader> pMultiVertexShader;
		std::shared_ptr<PixelShader> pMultiPixelShader;
		std::shared_ptr<class DepthStencilState> m_pNoDepthWrite;
		std::shared_ptr<class RasterizerState> m_pCullFront;
		std::shared_ptr<class DepthStencilState> m_pGreaterOrEqual;
		std::shared_ptr<class PixelCBuffer<PERSPECTIVEBUFFER>>	m_pPerspecCBuffer;
		std::shared_ptr<class BlendState>	m_pAccBlend;
		std::shared_ptr<class VertexShader> pPointVertexShader;
		std::shared_ptr<class HullShader> pPointHullShader;
		std::shared_ptr<class DomainShader> pPointDomainShader;
		std::shared_ptr<class VertexShader>	pShadowVertexShader;
		std::shared_ptr<class VertexShader>	pAnimShadowVertexShader;
		std::shared_ptr<class PixelShader>	pShadowPixelShader;
		std::shared_ptr<class MRT> pDepthBuffer[static_cast<int>(LIGHT_TYPE::END)];
		std::shared_ptr<class VertexCBuffer<TRANSFORMBUFFER>>	m_pTransformBuffer;

	public:
		void AddLight(const std::shared_ptr<PointLight>& pLight);
		void AddDrawable(const std::shared_ptr<Drawable>& pDrawable, int iLayer = 0);
		std::shared_ptr<class MRT> GetMRT()	const;
		std::shared_ptr<MRT> GetDepthBuffer(LIGHT_TYPE eType)	const;

	public:
		bool Init();
		void Update(float fDeltaTime);
		void PreRender();
		void Render();
		void RenderOpaque();
		void RenderOpaqueInst();
		void RenderAlpha();
		void RenderLight();
		void RenderShadow();
		void Clear();
	};

}
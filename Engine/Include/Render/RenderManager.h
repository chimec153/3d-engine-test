#pragma once

#include "../Core/Ptr.h"

namespace Engine
{
	template <typename T>
	class ConstantBuffer;

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
		std::list<class std::shared_ptr<class Drawable>>	m_RenderList[static_cast<int>(RENDER_LAYER::END)];
		std::list<class std::shared_ptr<class Drawable>>	m_ShadowList;
		std::unordered_map<size_t, class std::shared_ptr<class RenderInstancing>>	m_mapInstance[static_cast<int>(RENDER_LAYER::END)];
		std::unordered_map<size_t, class std::shared_ptr<class RenderInstancing>>	m_mapShadowInstance;

	private:
		std::shared_ptr<class MRT> pMRT;
		std::shared_ptr<class MRT> m_pDecalMRT;
#ifdef _DEBUG
		std::shared_ptr<class VertexShader> pNullVertexShader;
		std::shared_ptr<class PixelShader> pNullPixelShader;
#endif
		std::shared_ptr<VertexShader> pMultiVertexShader;
		std::shared_ptr<PixelShader> pMultiPixelShader;
		std::shared_ptr<class DepthStencilState> m_pNoDepthWrite;
		std::shared_ptr<class RasterizerState> m_pCullFront;
		std::shared_ptr<class DepthStencilState> m_pGreaterOrEqual;
		std::shared_ptr<class ConstantBuffer<PERSPECTIVEBUFFER>>	m_pPerspecCBuffer;
		std::shared_ptr<class BlendState>	m_pAccBlend;
		std::shared_ptr<class VertexShader> pPointVertexShader;
		std::shared_ptr<class HullShader> pPointHullShader;
		std::shared_ptr<class DomainShader> pPointDomainShader;
		std::shared_ptr<class VertexShader>	pShadowVertexShader;
		std::shared_ptr<class VertexShader>	pAnimShadowVertexShader;
		std::shared_ptr<class PixelShader>	pShadowPixelShader;
		std::shared_ptr<class MRT> pDepthBuffer[static_cast<int>(LIGHT_TYPE::END)];
		std::shared_ptr<class ConstantBuffer<TRANSFORMBUFFER>>	m_pTransformBuffer;
		std::shared_ptr<class BlendState>	m_pDecalBlend;
		std::shared_ptr<class DepthStencilState> m_pNoDepthRead;
		std::shared_ptr<class SkyBox>	m_pSkyBox;
		std::shared_ptr<class BlendState>	m_pAlphaBlend;
		std::shared_ptr<class StructuredBuffer>	m_pLightBuffer;
		std::shared_ptr<class StructuredBuffer>	m_pAverageLightBuffer;
		std::shared_ptr<class StructuredBuffer>	m_pPrevAverageLightBuffer;
		std::shared_ptr<class ConstantBuffer<DOWNSCALECBUFFER>> m_pDownScaleCBuffer;
		std::shared_ptr<class ConstantBuffer<HDRCBUFFER>> m_pHDRCBuffer;
		std::shared_ptr<class ComputeShader> m_pDownScaleFirstCS;
		std::shared_ptr<class ComputeShader> m_pDownScaleSecondCS;
		std::shared_ptr<class ComputeShader> m_pBrightCS;
		std::shared_ptr<class ComputeShader> m_pBloomVerticalFilterCS;
		std::shared_ptr<class ComputeShader> m_pBloomHorizontalFilterCS;
		std::shared_ptr<class MRT> m_pHDRTexture;
		std::shared_ptr<class Texture> m_pHDRDownScaleTexture;
		std::shared_ptr<class Texture> m_pBloomTexture;
		std::shared_ptr<class Texture> m_pBloomFinalTexture;
		std::shared_ptr<class PixelShader> m_pHDRPS; 
		HDRCBUFFER m_tHDRCBuffer;
		DOWNSCALECBUFFER m_tDownScaleCBuffer;
		std::shared_ptr<class MRT> m_pBlurTarget;
		std::shared_ptr<class ComputeShader> m_pBlurCS;
		std::shared_ptr<class Texture> m_pBlurTexture;
		std::shared_ptr<class VertexShader> pBlurNullVertexShader;
		std::shared_ptr<class PixelShader> pBlurNullPixelShader;
		std::shared_ptr<class BlendState> m_pDestAlpha;
		FOGCBUFFER m_tFogCBuffer;
		std::shared_ptr<class ConstantBuffer<FOGCBUFFER>> m_pFogCBuffer;

	public:
		void SetSkyBox(std::shared_ptr<SkyBox> pSkyBox);
		void SetHDRMidGray(float fMidGray);
		void SetHDRWhiteSqr(float fWhiteSqr);
		void SetBloomScale(float fScale);
		void SetBloomThreshold(float fThreshold);
		void SetFOVValueX(float fX);
		void SetFOVValueY(float fY);
		float GetHDRMidGray()	const;
		float GetHDRWhiteSqr()	const;
		float GetBloomScale()	const;
		float GetBloomThreshold()	const;
		float GetFOVValueX()	const;
		float GetFOVValueY()	const;
		std::shared_ptr<class Texture> GetHDRDownScaleTexture()	const;
		std::shared_ptr<class Texture> GetBloomTexture()	const;
		std::shared_ptr<class Texture> GetBloomFinalTexture()	const;
		void SetFogColor(const Vector3& vColor);
		void SetFogHighlightColor(const Vector3& vColor);
		void SetFogStartDepth(float fDepth);
		void SetFogDensity(float fDensity);
		void SetFogHeightFallOff(float fHeightFallOff);
		const Vector3& GetFogColor() const;
		const Vector3& GetFogHighlightColor() const;
		float GetFogStartDepth() const;
		float GetFogDensity() const;
		float GetFogHeightFallOff() const;

	public:
		void AddLight(const std::shared_ptr<PointLight>& pLight);
		void AddDrawable(const std::shared_ptr<Drawable>& pDrawable);
		std::shared_ptr<class MRT> GetMRT()	const;
		std::shared_ptr<MRT> GetDepthBuffer(LIGHT_TYPE eType)	const;
		std::shared_ptr<MRT> GetDecalMRT()	const;

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
		void RenderDecal();
		void RenderSkyBox();
		void RenderBlur();
		void RenderUI();
		void PostProcessing();
		void Clear();
#ifdef _DEBUG
		void RenderDebug();
#endif

	public:
		void Bloom();

	public:
		void HDRDownScaleFirst();
		void HDRDownScaleSecond();
		void RenderHDR();
		void Bright();
		void BloomFilter();
	};

}

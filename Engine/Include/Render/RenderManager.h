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
		// Phase E5 — m_RenderList<Drawable>, m_ShadowList, m_mapInstance,
		// m_mapShadowInstance removed. With AddDrawable gone (E5) and all
		// game classes migrated to GameObject + MeshRendererComponent,
		// these structures stayed empty and only added boilerplate.
		// Phase E5 — MeshRenderer instancing buckets. GameObjects with a
		// MeshRendererComponent self-register here every PreDraw, keyed
		// by GetInstanceKey so render passes can collapse duplicate-state
		// entities into a single DrawInstanced call when the bucket size
		// warrants it AND the "Inst" shader / input-layout variants are
		// registered. When the conditions don't hold (per-instance
		// Animation, decorator components like PaperBurn, missing Inst
		// variants), the bucket is rendered as individual draws.
		std::unordered_map<size_t, std::list<class std::shared_ptr<class MeshRendererComponent>>> m_mapMeshInstance[static_cast<int>(RENDER_LAYER::END)];

		// Phase E5 — Decal Component registration path. Replaces the
		// Drawable-subclass Decal that auto-registered into m_RenderList[DECAL].
		// Cleared each frame.
		std::list<class std::shared_ptr<class Decal>> m_DecalList;

		// Phase E5 — Particle Component registration path. Replaces the
		// Drawable-subclass Particle that auto-registered into
		// m_RenderList[layer]. Indexed by RENDER_LAYER (currently ALPHA
		// and BLUR are the consumers). Cleared each frame.
		std::list<class std::shared_ptr<class Particle>> m_ParticleList[static_cast<int>(RENDER_LAYER::END)];

		// Phase E5 — Generic per-layer render callback list. Any Component
		// (Engine-side or Client-side) can register a Bind-callback for
		// the given layer; the corresponding render pass invokes them in
		// registration order. Lets us avoid RenderManager depending on
		// downstream component types like Client::Trail. Cleared each
		// frame.
		std::list<std::function<void()>> m_CustomRenderList[static_cast<int>(RENDER_LAYER::END)];

	private:
		std::shared_ptr<class MRT> pMRT;
		std::shared_ptr<class MRT> m_pDecalMRT;
		// Unreal-style CustomDepth target. Depth-only MRT (no color RTs)
		// that flagged MeshRendererComponents draw into during
		// RenderCustomDepth(). Sampled in CompositeCustomDepth() to detect
		// pixels where the flagged mesh is behind opaque scene geometry.
		std::shared_ptr<class MRT> m_pCustomDepth;
		std::shared_ptr<class PixelShader> m_pCustomDepthCompositePS;
		std::shared_ptr<class BlendState> m_pCustomDepthCompositeBlend;
		std::shared_ptr<class DepthStencilState> m_pCustomDepthWriteState;
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
		// Multiplier applied to fDeltaTime when computing per-frame
		// adaptation step. 1.0 = engine default (slow); larger = faster
		// eye-adapt recovery from dark-to-bright transitions.
		float m_fAdaptationSpeed;
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
		void SetAdaptationSpeed(float fSpeed);
		float GetAdaptationSpeed() const;
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
		void AddMeshRenderer(const std::shared_ptr<class MeshRendererComponent>& pMR);
		void AddDecalComponent(const std::shared_ptr<class Decal>& pDecal);
		void AddParticle(const std::shared_ptr<class Particle>& pParticle);
		// Phase E5 — generic Component render callback for the given layer.
		// Used by Client-side components (Trail, etc.) so RenderManager
		// doesn't need to know their concrete types.
		void AddCustomRender(RENDER_LAYER eLayer, std::function<void()> renderFn);
		// Phase E5 — AddDrawable removed (no more live Drawable instances).
		std::shared_ptr<class MRT> GetMRT()	const;
		std::shared_ptr<MRT> GetDepthBuffer(LIGHT_TYPE eType)	const;
		std::shared_ptr<MRT> GetDecalMRT()	const;
		std::shared_ptr<MRT> GetCustomDepth()	const;

	public:
		bool Init();
		void Update(float fDeltaTime);
		void PreRender();
		void Render();
		void RenderOpaque();
		void RenderOpaqueInst();
		void RenderCustomDepth();
		void CompositeCustomDepth();
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

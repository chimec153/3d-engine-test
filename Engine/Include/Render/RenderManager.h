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
		std::list<class std::shared_ptr<class LightComponent>>	m_LightList[static_cast<int>(LIGHT_TYPE::END)];
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

		// Phase E5+ — first-class UIRenderer registration. UIRenderer is
		// Engine-side so RenderManager can know its type directly (the
		// CustomRender callback path stays for downstream/Client-side
		// pieces like Trail / DamageText / EnemyCountHUD). PreDraw pushes
		// here, RenderUI iterates and calls Bind() with no std::function
		// indirection. Cleared each frame in Clear().
		std::list<std::weak_ptr<class UIRenderer>> m_UIList;

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
		// UE CustomStencil: D24S8 타겟에 depth+stencil REPLACE. Bindable 래퍼
		// 대신 native ID3D11DepthStencilState — 매 MR마다 StencilRef를 바꿔
		// OMSetDepthStencilState 직접 호출하기 위함(기존 DSS 래퍼는 ref 상수).
		CPtr<ID3D11DepthStencilState> m_pCustomDepthStencilDSS;
		// UE outline post-process material 대응 패스 리소스.
		std::shared_ptr<class PixelShader>                       m_pOutlinePS;
		std::shared_ptr<class ConstantBuffer<OUTLINECBUFFER>>    m_pOutlineCBuffer;
		std::shared_ptr<class BlendState>                        m_pOutlineBlend;
		std::shared_ptr<class DepthStencilState>                 m_pOutlineDSS;
		OUTLINECBUFFER                                           m_tOutlineCBuffer;
#ifdef _DEBUG
		std::shared_ptr<class VertexShader> pNullVertexShader;
		std::shared_ptr<class PixelShader> pNullPixelShader;
#endif
		std::shared_ptr<class VertexShader> pMultiVertexShader;
		std::shared_ptr<class PixelShader> pMultiPixelShader;
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

		// Boss-death radial shockwaves. Animated CPU-side in Update() and
		// uploaded to b13; HDR.fx FinalPassPS warps the tonemap resolve
		// along each expanding ring. Entries self-expire when fAge >= fLife.
		struct ShockwaveInst
		{
			Vector2 vCenterUV;
			float   fAge;
			float   fLife;
			float   fMaxRadius;
			float   fAmplitude;
		};
		std::vector<ShockwaveInst> m_Shockwaves;
		SHOCKWAVECBUFFER m_tShockwaveCBuffer;
		std::shared_ptr<class ConstantBuffer<SHOCKWAVECBUFFER>> m_pShockwaveCBuffer;

		// Player damage-feedback overlays, packed into the same b13 cbuffer.
		// Gameplay sets these via the Set*/Add* methods below; UpdateShockwaves
		// decays the transient ones and uploads them with the shockwave data.
		float m_fDamageFlash = 0.f;   // sharp single-hit red flash (decays)
		float m_fChipTarget  = 0.f;   // chip-red set each contact frame (target)
		float m_fChipRed     = 0.f;   // chip-red actual (eases toward target, decays)
		float m_fLowHp       = 0.f;   // low-HP strength, held by gameplay
		float m_fFxTime      = 0.f;   // free-running clock for the vignette pulse
		float m_fHealFlash   = 0.f;   // green heal-flash (decays, max-merged)

		// Per-frame instancing tally — each successful instanced bucket
		// appends its member count here. The vector preserves render
		// order, which is the sort-by-(VS,PS,Material) order from
		// RenderOpaque, so identical entity types stay grouped. Cleared
		// in Clear() so the contents reflect the current frame only.
		std::vector<int> m_InstancedBucketCounts;

#ifdef _DEBUG
		// Debug wireframe overlay — each Collider's PreDraw pushes its edges
		// (already in world space) into m_DebugLineVertices when the toggle
		// is on; FlushDebugLines runs after RenderUI so wireframes overlay
		// the final image without participating in HDR / post-process.
		bool m_bDebugDrawColliders = false;
		std::vector<Vector3> m_DebugLineVertices;
		CPtr<ID3D11Buffer> m_pDebugLineVB;
		unsigned int m_iDebugLineVBCapacity = 0;
		std::shared_ptr<class VertexShader> m_pDebugLineVS;
		std::shared_ptr<class PixelShader>  m_pDebugLinePS;
		std::shared_ptr<class InputLayout>  m_pDebugLineIL;
		std::shared_ptr<class Topology>     m_pDebugLineTopology;
		std::shared_ptr<class Material>     m_pDebugLineMaterial;
#endif

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

		// 전역 씬 앰비언트 (라이트 선택과 무관). Fog cbuffer(b12)에 함께 실어
		// PS_Multi의 디퓨즈 앰비언트로 사용. 색/세기 분리.
		void SetAmbientColor(const Vector3& vColor);
		void SetAmbientIntensity(float fIntensity);
		Vector3 GetAmbientColor() const;
		float GetAmbientIntensity() const;

	public:
		void AddLight(const std::shared_ptr<LightComponent>& pLight);
		void AddMeshRenderer(const std::shared_ptr<class MeshRendererComponent>& pMR);
		void AddDecalComponent(const std::shared_ptr<class Decal>& pDecal);
		void AddParticle(const std::shared_ptr<class Particle>& pParticle);
		// Phase E5 — generic Component render callback for the given layer.
		// Used by Client-side components (Trail, etc.) so RenderManager
		// doesn't need to know their concrete types.
		void AddCustomRender(RENDER_LAYER eLayer, std::function<void()> renderFn);
		void AddUIRenderer(const std::shared_ptr<class UIRenderer>& p);
		// Spawn a radial screen-distortion shockwave centred on a world
		// point (projected to screen UV via the active camera). Used for
		// boss deaths — the ring expands and fades over fLife seconds.
		void AddShockwave(const Vector3& vWorldPos, float fLife = 0.35f,
			float fMaxRadius = 0.5f, float fAmplitude = 0.02f);

		// Player damage-feedback overlays (HDR.fx FinalPassPS, b13).
		//   AddDamageFlash — sharp full-screen red flash for a single big hit;
		//                    it decays each frame. Max-merged so a stronger
		//                    flash wins.
		//   SetChipRed     — call each frame contact/DoT damage lands; the
		//                    subtle red edge eases up to this and fades when
		//                    the calls stop (prevents per-tick strobing).
		//   SetLowHp       — held 0..1 low-HP strength (vignette + desaturate).
		void AddDamageFlash(float fStrength);
		void SetChipRed(float fStrength);
		void SetLowHp(float fStrength);
		//   AddHealFlash   — green full-screen flash when the player receives a
		//                    heal pulse. Mirrors AddDamageFlash (max-merged,
		//                    decays CPU-side); the green/red contrast keeps the
		//                    damage-vs-heal visual language consistent.
		void AddHealFlash(float fStrength);
		// Phase E5 — AddDrawable removed (no more live Drawable instances).
		std::shared_ptr<class MRT> GetMRT()	const;

		// Per-bucket instanced render counts for the just-finished frame.
		// Each entry = one bucket that took the DrawInstanced fast path.
		// Sum of entries == total instanced MR count. Cleared in Clear()
		// (post-Render) so reading during the next frame's UI render
		// shows the just-finished frame's values.
		const std::vector<int>& GetInstancedBucketCounts() const { return m_InstancedBucketCounts; }
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
		// UE의 outline post-process material과 동일 위치(After Tonemapping).
		// 풀스크린 PS가 CustomDepth+Stencil sobel 후 backbuffer에 알파블렌드.
		void RenderOutline();

	public:
		// 외곽선 색·두께 런타임 조정용 (UE PP material 스칼라/벡터 파라미터 대응).
		void SetOutlineColor(const Vector4& vColor)    { m_tOutlineCBuffer.vOutlineColor    = vColor; }
		void SetOutlineColorAlt(const Vector4& vColor) { m_tOutlineCBuffer.vOutlineColorAlt = vColor; }
		void SetOutlineThickness(int iThickness)       { m_tOutlineCBuffer.iThickness = iThickness; }
		void RenderAlpha();
		void RenderLight();
		void RenderShadow();
		void RenderDecal();
		void RenderSkyBox();
		void RenderBlur();
		void RenderUI();
		void PostProcessing();
		void UpdateShockwaves(float fDeltaTime);
		void Clear();
#ifdef _DEBUG
		void RenderDebug();

		// Collider wireframe overlay. Toggle from ImGui; AddDebugLine is
		// called from each Collider's PreDraw (world-space endpoint pairs).
		// FlushDebugLines runs at the tail of Render().
		void SetDebugDrawColliders(bool b) { m_bDebugDrawColliders = b; }
		bool IsDebugDrawColliders() const  { return m_bDebugDrawColliders; }
		void AddDebugLine(const Vector3& p0, const Vector3& p1);

	private:
		void InitDebugLines();
		void FlushDebugLines();
	public:
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

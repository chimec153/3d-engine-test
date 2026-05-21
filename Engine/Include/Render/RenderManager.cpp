#include "RenderManager.h"
#include "MRT.h"
#include "../Core/Window.h"
#include "../Bindable/PointLight.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/BlendState.h"
#include "../Bindable/HullShader.h"
#include "../Bindable/DomainShader.h"
#include "../Bindable/DepthStencilState.h"
#include "../Bindable/RasterizerState.h"
#include "../Bindable/Transform.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/Camera.h"
#include "../Bindable/PointLight.h"
#include "../Bindable/IndexBuffer.h"
#include "../Bindable/VertexBuffer.h"
#include "../Bindable/Texture.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/Material.h"
#include "../Bindable/Mesh.h"
#include "../Bindable/Animation.h"
#include "../Animation/Skeleton.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/SkyBox.h"
#include "../Bindable/ComputeShader.h"
#include "../Shader/StructuredBuffer.h"
#include "../Core/Graphics.h"
#include "../Component/MeshRendererComponent.h"
#include "../GameObject/GameObject.h"
#include "../Bindable/Decal.h"
#include "../Bindable/Particle.h"
#include "../Bindable/PaperBurn.h"
#include <algorithm>

namespace
{
	// Phase E5 — render a single MeshRendererComponent solo. Mirrors the
	// per-MR loop body in RenderOpaque/RenderAlpha — extracted into a
	// helper so the bucket-iteration code can fall back to it cleanly.
	void RenderSoloMR(const std::shared_ptr<Engine::MeshRendererComponent>& pMR)
	{
		if (!pMR) return;
		Engine::GameObject* pOwner = pMR->GetGameObjectOwner();
		std::shared_ptr<Engine::Transform> pTr =
			pOwner ? pOwner->GetComponent<Engine::Transform>() : nullptr;
		if (pTr) pTr->Bind();
		pMR->Bind();
		if (pMR->GetMesh()) pMR->GetMesh()->Draw(pMR->MakeMaterialResolver());
		pMR->PostBind();
		if (pTr) pTr->PostBind();
	}

	// Phase E5 — true DrawInstanced fast path for a homogeneous bucket
	// (all members share Mesh/VS/PS/Material; none have Animation or
	// decorator Components like PaperBurn that bind per-instance state).
	// Returns true if the instanced draw was issued; false means the
	// caller must fall back to RenderSoloMR for each member.
	bool TryRenderInstancedBucket(const std::list<std::shared_ptr<Engine::MeshRendererComponent>>& bucket)
	{
		if (bucket.size() < 2) return false;

		const auto& pFirst = bucket.front();
		if (!pFirst || !pFirst->GetMesh() || !pFirst->GetVertexShader())
			return false;

		// Animation / decorator components break shared-state instancing
		// (per-instance bone palette, per-instance PaperBurn CB, etc.).
		if (pFirst->GetAnimation()) return false;
		if (auto* pOwner = pFirst->GetGameObjectOwner())
		{
			for (const auto& pSibling : pOwner->GetComponentList())
			{
				if (pSibling.get() == pFirst.get()) continue;
				// PaperBurn is the only RenderBind-overriding decorator
				// today; conservatively bail on any non-trivial sibling.
				if (std::dynamic_pointer_cast<Engine::PaperBurn>(pSibling))
					return false;
			}
		}

		auto pInstVS = Engine::StaticFindBindable<Engine::VertexShader>(
			pFirst->GetVertexShader()->GetTag() + "Inst");
		if (!pInstVS) return false;
		auto pInstIL = pInstVS->GetInstInputLayout();
		if (!pInstIL) return false;
		const int iInstSize = pInstIL->GetInstSize();
		if (iInstSize <= 0) return false;

		// Build per-instance data buffer.
		const int iCount = static_cast<int>(bucket.size());
		std::vector<char> data(static_cast<size_t>(iInstSize) * iCount, 0);
		int idx = 0;
		for (const auto& pMR : bucket)
		{
			if (pMR) pMR->GetInstData(&data[idx * iInstSize], iInstSize);
			++idx;
		}

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(iInstSize) * iCount;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = data.data();

		Engine::CPtr<ID3D11Buffer> pInstBuffer;
		HRESULT hr = Engine::Graphics::GetInst()->GetDevice()->CreateBuffer(
			&desc, &initData, &pInstBuffer);
		if (FAILED(hr)) return false;

		// Bind shared GPU state from the first MR (consistent across the
		// bucket since GetInstanceKey hashes Mesh/VS/PS/Material/Anim).
		pInstIL->Bind();
		pInstVS->Bind();
		if (pFirst->GetPixelShader()) pFirst->GetPixelShader()->Bind();
		if (pFirst->GetMaterial())    pFirst->GetMaterial()->Bind();
		for (const auto& tex : pFirst->GetTextures()) if (tex) tex->Bind();
		for (const auto& b : pFirst->GetOtherBindables())
		{
			if (b && (b->GetObjectType() == Engine::OBJECT_TYPE::BIND ||
			          b->GetObjectType() == Engine::OBJECT_TYPE::COLLIDER))
				b->Bind();
		}

		pFirst->GetMesh()->DrawInst(iCount, iInstSize, pInstBuffer, pFirst->MakeMaterialResolver());

		if (pFirst->GetPixelShader()) pFirst->GetPixelShader()->PostBind();
		if (pFirst->GetMaterial())    pFirst->GetMaterial()->PostBind();
		pInstVS->PostBind();
		return true;
	}
}

namespace Engine
{
	RenderManager* RenderManager::m_pInst = nullptr;

	RenderManager::RenderManager()
	{
	}

	RenderManager::~RenderManager()
	{
		// Phase E7 — RenderV2 retired; ShutdownDemo no longer needed.
	}

	void RenderManager::SetSkyBox(std::shared_ptr<SkyBox> pSkyBox)
	{
		m_pSkyBox = pSkyBox;
	}

	void RenderManager::SetHDRMidGray(float fMidGray)
	{
		m_tHDRCBuffer.fMiddleGray = fMidGray;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::AddLight(const std::shared_ptr<PointLight>& pLight)
	{
		m_LightList[static_cast<int>(pLight->GetLightType())].push_back(pLight);
	}

	void RenderManager::AddMeshRenderer(const std::shared_ptr<MeshRendererComponent>& pMR)
	{
		if (!pMR) return;
		int iLayer = static_cast<int>(pMR->GetRenderLayer());
		assert(iLayer >= 0 && iLayer < static_cast<int>(RENDER_LAYER::END));
		m_mapMeshInstance[iLayer][pMR->GetInstanceKey()].push_back(pMR);
	}

	void RenderManager::AddDecalComponent(const std::shared_ptr<Decal>& pDecal)
	{
		if (!pDecal) return;
		m_DecalList.push_back(pDecal);
	}

	void RenderManager::AddParticle(const std::shared_ptr<Particle>& pParticle)
	{
		if (!pParticle) return;
		int iLayer = static_cast<int>(pParticle->GetRenderLayer());
		assert(iLayer >= 0 && iLayer < static_cast<int>(RENDER_LAYER::END));
		m_ParticleList[iLayer].push_back(pParticle);
	}

	void RenderManager::AddCustomRender(RENDER_LAYER eLayer, std::function<void()> renderFn)
	{
		int iLayer = static_cast<int>(eLayer);
		assert(iLayer >= 0 && iLayer < static_cast<int>(RENDER_LAYER::END));
		m_CustomRenderList[iLayer].push_back(std::move(renderFn));
	}

	void RenderManager::SetHDRWhiteSqr(float fWhiteSqr)
	{
		m_tHDRCBuffer.fLumWhiteSqr = fWhiteSqr;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	// Phase E5 — RenderManager::AddDrawable removed. With all 25 Drawable
	// subclasses migrated to Component / GameObject, no live runtime path
	// creates a Drawable instance; Drawable::PreDraw no longer self-
	// registers here, so this function had no callers. The render passes
	// still iterate m_RenderList[layer] / m_ShadowList / m_mapInstance,
	// but those containers stay empty in a clean run — they'll be removed
	// outright in the final E7 cleanup along with the Drawable class
	// itself.

	float RenderManager::GetHDRMidGray() const
	{
		return m_tHDRCBuffer.fMiddleGray;
	}

	std::shared_ptr<class MRT> RenderManager::GetMRT() const
	{
		return pMRT;
	}

	float RenderManager::GetHDRWhiteSqr() const
	{
		return m_tHDRCBuffer.fLumWhiteSqr;
	}

	std::shared_ptr<MRT> RenderManager::GetDepthBuffer(LIGHT_TYPE eType) const
	{
		return pDepthBuffer[static_cast<int>(eType)];
	}

	std::shared_ptr<MRT> RenderManager::GetDecalMRT() const
	{
		return m_pDecalMRT;
	}

	std::shared_ptr<MRT> RenderManager::GetCustomDepth() const
	{
		return m_pCustomDepth;
	}

	bool RenderManager::Init()
	{
		// Phase E7 — RenderV2 init removed. Sort-by-state will be reintroduced
		// inside the V1 render path in a follow-up.

		// MRT0~3: albedo/normal/specMap/specColor (RGBA8 each).
		// MRT4: per-pixel emissive (R11G11B10_FLOAT for HDR in 4B).
		// MRT4's SRV is rebound to t18 in RenderLight() because t15 is Shadow.
		const std::vector<DXGI_FORMAT>& format = {
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R11G11B10_FLOAT,
		};

		pMRT = std::make_shared<MRT>(format, 11);

		if (pMRT == nullptr)
		{
			return false;
		}

		pMRT->SetTag("MRT");

		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
		{
			const std::vector<DXGI_FORMAT>& format = { DXGI_FORMAT_R8G8B8A8_UNORM };

			pDepthBuffer[i] = std::make_shared<MRT>(format, 0, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);

			char name[TEXT_LEN];

			sprintf_s(name, "DepthBuffer %d", i);

			pDepthBuffer[i]->SetTag(name);

			pDepthBuffer[i]->Clear();
		}

		m_pDecalMRT = std::make_shared<MRT>(format, 25);

		m_pDecalMRT->SetTag("DecalMRT");

		// Phase V8 — CustomDepth target. Depth-only, no color RTs. Same
		// resolution as the main back buffer (MRT auto-sizes from Window).
		// Sampled at t19 in CompositeCustomDepth().
		{
			const std::vector<DXGI_FORMAT> noColor = {};
			m_pCustomDepth = std::make_shared<MRT>(noColor, 0,
				DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT);
			if (m_pCustomDepth) m_pCustomDepth->SetTag("CustomDepth");
		}

#ifdef _DEBUG
		pNullVertexShader = StaticFindBindable<VertexShader>("NullVS");

		if (pNullVertexShader == nullptr)
		{
			return false;
		}

		pNullPixelShader = StaticFindBindable<PixelShader>("NullPS");

		if (pNullPixelShader == nullptr)
		{
			return false;
		}
#endif

		pMultiVertexShader = StaticFindBindable<VertexShader>("MultiVS");

		if (pMultiVertexShader == nullptr)
		{
			return false;
		}

		pMultiPixelShader = StaticFindBindable<PixelShader>("MultiPS");

		if (pMultiPixelShader == nullptr)
		{
			return false;
		}

		pPointVertexShader = StaticFindBindable<VertexShader>("PointLightVS");

		if (!pPointVertexShader)
		{
			return false;
		}

		pPointHullShader = StaticFindBindable<HullShader>("PointLightHS");

		if (pPointHullShader == nullptr)
		{
			return false;
		}

		pPointDomainShader = StaticFindBindable<DomainShader>("PointLightDS");

		if (pPointDomainShader == nullptr)
		{
			return false;
		}

		pShadowVertexShader = StaticFindBindable<VertexShader>("ShadowVS");

		if (!pShadowVertexShader)
		{
			return false;
		}

		pAnimShadowVertexShader = StaticFindBindable<VertexShader>("ShadowAnimVS");

		if (!pAnimShadowVertexShader)
		{
			return false;
		}

		pShadowPixelShader = StaticFindBindable<PixelShader>("ShadowPS");

		if (!pShadowPixelShader)
		{
			return false;
		}

		m_pNoDepthWrite = StaticFindBindable<DepthStencilState>("NoDepthWrite");

		if (m_pNoDepthWrite == nullptr)
		{
			return false;
		}

		m_pCullFront = StaticFindBindable<RasterizerState>("CullFront");

		if (!m_pCullFront)
		{
			return false;
		}

		m_pGreaterOrEqual = StaticFindBindable<DepthStencilState>("Greater");

		if (!m_pGreaterOrEqual)
		{
			return false;
		}

		m_pPerspecCBuffer = StaticFindBindable<ConstantBuffer<PERSPECTIVEBUFFER>>("Perspective");

		if (m_pPerspecCBuffer == nullptr)
		{
			return false;
		}

		m_pAccBlend = StaticFindBindable<BlendState>("AccBlend");

		if (!m_pAccBlend)
		{
			return false;
		}

		m_pTransformBuffer = StaticFindBindable<ConstantBuffer<TRANSFORMBUFFER>>("Transform");

		if (!m_pTransformBuffer)
		{
			return false;
		}

		m_pDecalBlend = StaticFindBindable<BlendState>("DecalBlend");

		if (!m_pDecalBlend) 
		{
			return false;
		}

		m_pNoDepthRead = StaticFindBindable<DepthStencilState>("NoDepth");

		if (!m_pNoDepthRead)
		{
			return false;
		}

		m_pAlphaBlend = StaticFindBindable<BlendState>("AlphaBlend");

		if (!m_pAlphaBlend)
		{
			return false;
		}

		m_pLightBuffer = std::make_shared<StructuredBuffer>(Window::GetInst()->GetWidth() * Window::GetInst()->GetHeight() / (16 * 1024), 4);

		m_pAverageLightBuffer = std::make_shared<StructuredBuffer>(1, 4);

		m_pPrevAverageLightBuffer = std::make_shared<StructuredBuffer>(1, 4);

		m_pDownScaleCBuffer = std::make_shared<ConstantBuffer<DOWNSCALECBUFFER>>();

		m_pHDRCBuffer = std::make_shared<ConstantBuffer<HDRCBUFFER>>();

		m_tDownScaleCBuffer.iResX = Window::GetInst()->GetWidth() / 4;
		m_tDownScaleCBuffer.iResY = Window::GetInst()->GetHeight() / 4;

		m_tDownScaleCBuffer.iDomain = (Window::GetInst()->GetWidth() * Window::GetInst()->GetHeight()) / 16;

		m_tDownScaleCBuffer.iGroupSize = (Window::GetInst()->GetWidth() * Window::GetInst()->GetHeight()) / (16 * 1024);

		m_tDownScaleCBuffer.fBloomThreshold = 0.5f;

		m_fAdaptationSpeed = 1.f;

		m_pDownScaleCBuffer->UpdateBuffer(m_tDownScaleCBuffer);

		m_tHDRCBuffer.fMiddleGray = 0.25f;
		m_tHDRCBuffer.fLumWhiteSqr = 1.f;
		m_tHDRCBuffer.fBloomScale = 0.12f;
		m_tHDRCBuffer.vDOFFarValues.x = 500.f;
		m_tHDRCBuffer.vDOFFarValues.y = 0.5f;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);

		m_pDownScaleFirstCS = std::make_shared<ComputeShader>(TEXT("HDR.fx"), "DownScaleFirstPass");

		m_pDownScaleSecondCS = std::make_shared<ComputeShader>(TEXT("HDR.fx"), "DownScaleSecondPass");

		m_pBrightCS = std::make_shared<ComputeShader>(TEXT("HDR.fx"), "BrightPass");

		m_pBloomVerticalFilterCS = std::make_shared<ComputeShader>(TEXT("HDR.fx"), "VerticalFilter");

		m_pBloomHorizontalFilterCS = std::make_shared<ComputeShader>(TEXT("HDR.fx"), "HorizonFilter");

		m_pHDRTexture = std::make_shared<MRT>(std::vector<DXGI_FORMAT>({DXGI_FORMAT_R16G16B16A16_FLOAT}), 7);

		m_pHDRPS = std::make_shared<PixelShader>(TEXT("HDR.fx"), "FinalPassPS");

		m_pHDRDownScaleTexture = std::make_shared<Texture>(Window::GetInst()->GetWidth() / 4, Window::GetInst()->GetHeight() / 4, 2, DXGI_FORMAT_R32G32B32A32_FLOAT);

		m_pBloomTexture = std::make_shared<Texture>(Window::GetInst()->GetWidth() / 4, Window::GetInst()->GetHeight() / 4, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);

		m_pBloomFinalTexture = std::make_shared<Texture>(Window::GetInst()->GetWidth() / 4, Window::GetInst()->GetHeight() / 4, 3, DXGI_FORMAT_R32G32B32A32_FLOAT);

		m_pBlurTarget = std::make_shared<MRT>(std::vector<DXGI_FORMAT>({DXGI_FORMAT_R8G8B8A8_UNORM}) , 0);

		m_pBlurCS = std::make_shared<ComputeShader>(TEXT("Particle.fx"), "Blur");

		m_pBlurTexture = std::make_shared<Texture>(Window::GetInst()->GetWidth(), Window::GetInst()->GetHeight(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);

		pBlurNullVertexShader = std::make_shared<VertexShader>(TEXT("Particle.fx"), "NullVS");

		pBlurNullPixelShader = std::make_shared<PixelShader>(TEXT("Particle.fx"), "NullPS");

		m_pDestAlpha = StaticFindBindable<BlendState>("DestAlpha");

		// Phase V8 — CustomDepth composite resources. PS samples scene depth
		// (t10) + custom depth (t19) and outputs a silhouette when the
		// flagged mesh is occluded. AlphaBlend already exists; reuse it.
		// CustomDepthWrite: depth=LESS_EQUAL + write ALL, stencil off. The
		// "Basic" state already matches; reuse without registering a new one.
		m_pCustomDepthCompositePS    = StaticFindBindable<PixelShader>("CustomDepthCompositePS");
		m_pCustomDepthCompositeBlend = StaticFindBindable<BlendState>("AlphaBlend");
		m_pCustomDepthWriteState     = StaticFindBindable<DepthStencilState>("Basic");

		m_pFogCBuffer = std::make_shared<ConstantBuffer<FOGCBUFFER>>(12);

		if (!m_pFogCBuffer) 
		{
			return false;
		}

		m_tFogCBuffer.vFogColor = Vector3(0.f, 0.f, 1.f);
		m_tFogCBuffer.vFogHighlightColor = Vector3(1.f, 0.f, 0.f);
		m_tFogCBuffer.fFogStartDepth = 500.f;
		m_tFogCBuffer.fFogGlobalDensity = 1.f;
		m_tFogCBuffer.fFogHeightFallOff = 1.f;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);

		return true;
	}

	void RenderManager::Update(float fDeltaTime)
	{
		// Phase E5 — Drawable instancing maps removed. No live RenderInstancing
		// instances exist anymore (RenderManager::AddDrawable was the only
		// populator and it has been deleted). Component-side rendering paths
		// (MeshRenderer / Decal / Particle / CustomRender) handle their own
		// per-frame state.

		m_tDownScaleCBuffer.fAdaptation = fDeltaTime * m_fAdaptationSpeed;

		m_pDownScaleCBuffer->UpdateBuffer(m_tDownScaleCBuffer);
	}

	void RenderManager::PreRender()
	{
		// Phase E5 — Drawable instancing PreRender removed (no live
		// instances).
	}

	void RenderManager::SetBloomScale(float fScale)
	{
		m_tHDRCBuffer.fBloomScale = fScale;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::Render()
	{
		pMRT->Clear();

		m_pDecalMRT->Clear();

		RenderOpaque();

		// Phase V8 — Unreal-style CustomDepth. Flagged MRs draw to a
		// depth-only target right after the main opaque pass; the silhouette
		// is composited into m_pHDRTexture between RenderSkyBox and
		// RenderAlpha (below) so alpha layers blend over it correctly.
		RenderCustomDepth();

		RenderDecal();

		RenderShadow();

		m_pNoDepthWrite->Bind();

		m_pHDRTexture->SetTargets();

		RenderLight();

		RenderSkyBox();

		CompositeCustomDepth();

		RenderAlpha();

		m_pHDRTexture->ResetTargets();

		RenderBlur();

		PostProcessing();

		RenderUI();

		m_pNoDepthWrite->PostBind();

#ifdef _DEBUG
		//RenderDebug();
#endif

		// Phase E7 — RenderV2 flush block removed. The sort-by-state pass
		// will be re-added inside the V1 path (per RenderManager render-pass
		// orchestration) in a follow-up rather than as a separate queue.

		Clear();
	}

	void RenderManager::SetBloomThreshold(float fThreshold)
	{
		m_tDownScaleCBuffer.fBloomThreshold = fThreshold;

		m_pDownScaleCBuffer->UpdateBuffer(m_tDownScaleCBuffer);
	}

	void RenderManager::SetAdaptationSpeed(float fSpeed)
	{
		m_fAdaptationSpeed = fSpeed;
	}

	float RenderManager::GetAdaptationSpeed() const
	{
		return m_fAdaptationSpeed;
	}

	void RenderManager::RenderOpaque()
	{
		pMRT->SetTargets();

		// Invalidate the per-shader bound caches at pass entry — between
		// passes the actual GPU state is whatever the previous pass left,
		// which our trackers don't observe.
		Graphics::GetInst()->ResetBindCache();

		// Phase E7 — sort-by-state pass. m_mapMeshInstance is keyed by
		// MeshRendererComponent::GetInstanceKey() (a hash combining
		// Mesh/VS/PS/Material/Animation tags), so each bucket is
		// state-homogeneous. But unordered_map iteration is hash-bucket
		// order — adjacent buckets share little. Collect non-empty buckets
		// into a vector and sort by (VS ptr, PS ptr, Material ptr) so
		// buckets with the same shaders/material draw adjacently. The
		// VS/PS/Texture::Bind cache then skips the redundant SetShader /
		// SetTexture calls between adjacent same-state buckets.
		auto& mapBuckets = m_mapMeshInstance[static_cast<int>(RENDER_LAYER::OPACUE)];

		using BucketRef = std::list<std::shared_ptr<MeshRendererComponent>>*;
		using SortEntry = std::tuple<void*, void*, void*, BucketRef>;
		std::vector<SortEntry> sortedBuckets;
		sortedBuckets.reserve(mapBuckets.size());

		for (auto& kv : mapBuckets)
		{
			if (kv.second.empty()) continue;
			const auto& pFirst = kv.second.front();
			if (!pFirst) continue;
			void* vsPtr  = pFirst->GetVertexShader().get();
			void* psPtr  = pFirst->GetPixelShader().get();
			void* matPtr = pFirst->GetMaterial().get();
			sortedBuckets.emplace_back(vsPtr, psPtr, matPtr, &kv.second);
		}

		std::sort(sortedBuckets.begin(), sortedBuckets.end(),
			[](const SortEntry& a, const SortEntry& b)
			{
				return std::tie(std::get<0>(a), std::get<1>(a), std::get<2>(a))
				     < std::tie(std::get<0>(b), std::get<1>(b), std::get<2>(b));
			});

		// For each instance-key bucket, try the DrawInstanced fast path;
		// fall back to per-MR solo rendering when the bucket has Animation /
		// decorator Components / no "Inst" shader variant.
		for (auto& entry : sortedBuckets)
		{
			auto& bucket = *std::get<3>(entry);
			if (TryRenderInstancedBucket(bucket)) continue;
			for (const auto& pMR : bucket) RenderSoloMR(pMR);
		}

		pMRT->ResetTargets();
	}

	void RenderManager::SetFOVValueX(float fX)
	{
		m_tHDRCBuffer.vDOFFarValues.x = fX;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::RenderOpaqueInst()
	{
		// Phase E5 — Drawable opaque instancing removed (m_mapInstance gone).
	}

	void RenderManager::RenderCustomDepth()
	{
		if (!m_pCustomDepth) return;

		// Iterate OPACUE buckets first to see if any flagged MR exists. If
		// none, skip the entire pass — no clear, no state churn.
		auto& mapBuckets = m_mapMeshInstance[static_cast<int>(RENDER_LAYER::OPACUE)];
		bool bAny = false;
		for (auto& kv : mapBuckets)
		{
			if (kv.second.empty()) continue;
			const auto& pFirst = kv.second.front();
			if (pFirst && pFirst->IsCustomDepthEnabled()) { bAny = true; break; }
		}
		if (!bAny) return;

		m_pCustomDepth->Clear(D3D11_CLEAR_DEPTH);

		// Depth-only render: 0 color RTs + CustomDepth's DSV. The MRT
		// helper handles the 0-RT case (OMSetRenderTargets(0, nullptr, DSV)).
		m_pCustomDepth->SetTargets();

		// Standard depth-write state. CustomDepth doesn't care about the
		// MR's own DSS (e.g. OutLineMask's stencil writes) — we want a
		// clean LESS_EQUAL + write ALL.
		if (m_pCustomDepthWriteState) m_pCustomDepthWriteState->Bind();

		Graphics::GetInst()->ResetBindCache();

		for (auto& kv : mapBuckets)
		{
			if (kv.second.empty()) continue;
			const auto& pFirst = kv.second.front();
			if (!pFirst || !pFirst->IsCustomDepthEnabled()) continue;

			for (const auto& pMR : kv.second)
			{
				if (!pMR || !pMR->IsCustomDepthEnabled()) continue;

				GameObject* pOwner = pMR->GetGameObjectOwner();
				std::shared_ptr<Transform> pTr =
					pOwner ? pOwner->GetComponent<Transform>() : nullptr;
				if (pTr) pTr->Bind();

				// Bind VS + Animation (skinning bone palette) + IL/Topology
				// /Rasterizer only. Skip PS/Material/Textures/DSS — the
				// depth-only pass doesn't sample or shade.
				if (pMR->GetVertexShader()) pMR->GetVertexShader()->Bind();
				if (pMR->GetAnimation())    pMR->GetAnimation()->Bind();
				for (const auto& b : pMR->GetOtherBindables())
				{
					if (!b) continue;
					switch (b->GetBindableType())
					{
					case BINDABLE_TYPE::INPUTLAYOUT:
					case BINDABLE_TYPE::TOPOLOGY:
					case BINDABLE_TYPE::RASTERIZER_STATE:
						b->Bind();
						break;
					default:
						break;
					}
				}

				// Null PS → driver skips pixel work entirely; only depth
				// is written.
				Graphics::GetInst()->GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);

				if (pMR->GetMesh()) pMR->GetMesh()->Draw(pMR->MakeMaterialResolver());

				for (const auto& b : pMR->GetOtherBindables())
				{
					if (!b) continue;
					switch (b->GetBindableType())
					{
					case BINDABLE_TYPE::INPUTLAYOUT:
					case BINDABLE_TYPE::TOPOLOGY:
					case BINDABLE_TYPE::RASTERIZER_STATE:
						b->PostBind();
						break;
					default:
						break;
					}
				}
				if (pMR->GetAnimation())    pMR->GetAnimation()->PostBind();
				if (pMR->GetVertexShader()) pMR->GetVertexShader()->PostBind();
				if (pTr) pTr->PostBind();
			}
		}

		if (m_pCustomDepthWriteState) m_pCustomDepthWriteState->PostBind();

		m_pCustomDepth->ResetTargets();
	}

	void RenderManager::CompositeCustomDepth()
	{
		if (!m_pCustomDepth || !m_pCustomDepthCompositePS) return;

		// Check there's actually something to composite — skip when no
		// flagged MR drew this frame (mirrors RenderCustomDepth's early-out).
		auto& mapBuckets = m_mapMeshInstance[static_cast<int>(RENDER_LAYER::OPACUE)];
		bool bAny = false;
		for (auto& kv : mapBuckets)
		{
			if (kv.second.empty()) continue;
			const auto& pFirst = kv.second.front();
			if (pFirst && pFirst->IsCustomDepthEnabled()) { bAny = true; break; }
		}
		if (!bAny) return;

		// Fullscreen pass over m_pHDRTexture. Render() already binds it as
		// the active target (with pMRT's DSV) before RenderAlpha, so we
		// don't rebind RTs here — just sample depths and write color.
		pMRT->SetDepthSRV(10);                 // scene depth → t10
		m_pCustomDepth->SetDepthSRV(19);       // custom depth → t19

		if (m_pCustomDepthCompositeBlend) m_pCustomDepthCompositeBlend->Bind();

		auto* pCtx = Graphics::GetInst()->GetDeviceContext();
		pCtx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		pCtx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		pCtx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		if (pMultiVertexShader)             pMultiVertexShader->Bind();
		if (m_pCustomDepthCompositePS)      m_pCustomDepthCompositePS->Bind();

		pCtx->Draw(4, 0);

		if (m_pCustomDepthCompositePS)      m_pCustomDepthCompositePS->PostBind();
		if (pMultiVertexShader)             pMultiVertexShader->PostBind();
		if (m_pCustomDepthCompositeBlend)   m_pCustomDepthCompositeBlend->PostBind();

		pMRT->ResetSRV(10);
		m_pCustomDepth->ResetSRV(19);
	}

	float RenderManager::GetBloomScale() const
	{
		return m_tHDRCBuffer.fBloomScale;
	}

	void RenderManager::SetFOVValueY(float fY)
	{
		m_tHDRCBuffer.vDOFFarValues.y = fY;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::RenderAlpha()
	{
		// Pass boundary — invalidate the VS/PS bound caches.
		Graphics::GetInst()->ResetBindCache();

		m_pHDRTexture->ResetTargets();

		m_pHDRTexture->SetTargets(pMRT->GetDSV());

		//pMRT->SetDepthSRV(10);
		m_pAlphaBlend->Bind();

		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
		{
			std::list<std::shared_ptr<PointLight>>::iterator iterL = m_LightList[i].begin();
			std::list<std::shared_ptr<PointLight>>::iterator iterLEnd = m_LightList[i].end();

			pDepthBuffer[i]->SetDepthSRV(15);

			for (; iterL != iterLEnd; ++iterL)
			{
				(*iterL)->Bind();

				// Phase E5 — Drawable iterate / instancing removed (no live
				// Drawable instances).

				// Component-side particle pass for ALPHA layer.
				for (const auto& pParticle : m_ParticleList[static_cast<int>(RENDER_LAYER::ALPHA)])
				{
					if (pParticle) pParticle->Bind();
				}

				// Generic Component render callbacks for ALPHA layer.
				for (const auto& fn : m_CustomRenderList[static_cast<int>(RENDER_LAYER::ALPHA)])
				{
					if (fn) fn();
				}
			}

			pDepthBuffer[i]->ResetSRV(15);
		}

		m_pAlphaBlend->PostBind();
		//pMRT->ResetSRV(10);
	}

	void RenderManager::RenderLight()
	{
		const std::shared_ptr<Camera>& pCamera = Graphics::GetInst()->GetCamera();

		PERSPECTIVEBUFFER buffer = {};

		if (pCamera)
		{
			const Matrix& matProj = pCamera->GetProjectMatrix();

			buffer.vPerspective = { 1.f / matProj.ff[0][0],1.f / matProj.ff[1][1], matProj.ff[2][2], -matProj.ff[3][2] };

			const std::shared_ptr<Transform>& pTransform = pCamera->GetTransform();

			const std::shared_ptr<PointLight>& pLight = Graphics::GetInst()->GetLight();

			if (pTransform && pLight)
			{
				buffer.matInvView = pTransform->GetRotationTranslationMatrix();

				buffer.matCameraViewToLightClip = buffer.matInvView * pLight->GetViewProject();

				buffer.matInvView.Transpose();

				buffer.matCameraViewToLightClip.Transpose();
			}
		}

		m_pFogCBuffer->Bind();

		m_pPerspecCBuffer->UpdateBuffer(buffer);

		m_pPerspecCBuffer->Bind();

		m_pDecalMRT->SetSRV();
		pMRT->SetSRV();
		// SetSRV() binds 5 SRVs contiguously at t11~t15. t15 collides with
		// Shadow but is overwritten below by pDepthBuffer[i]->SetDepthSRV(15).
		// Also rebind MRT4 to t18 — that's where the shader expects emissive.
		pMRT->SetSRV(4, 18);
		pMRT->SetDepthSRV(10);

		m_pAccBlend->Bind();

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		pMultiVertexShader->Bind();

		pMultiPixelShader->Bind();

		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
		{
			pDepthBuffer[i]->SetDepthSRV(15);

			std::list<std::shared_ptr<PointLight>>::iterator iter = m_LightList[i].begin();
			std::list<std::shared_ptr<PointLight>>::iterator iterEnd = m_LightList[i].end();

			for (; iter != iterEnd; ++iter)
			{
				(*iter)->Bind();

				Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

				(*iter)->PostBind();

			}

			pDepthBuffer[i]->ResetSRV(15);
		}

		m_pAccBlend->PostBind();

		m_pDecalMRT->ResetSRV();
		pMRT->ResetSRV(10);
		pMRT->ResetSRV(18);
		pMRT->ResetSRV();

	}

	float RenderManager::GetBloomThreshold() const
	{
		return m_tDownScaleCBuffer.fBloomThreshold;
	}

	void RenderManager::RenderShadow()
	{
		const std::shared_ptr<PointLight>& pLight = Graphics::GetInst()->GetLight();

		if (!pLight)
		{
			return;
		}

		pDepthBuffer[2]->Clear(D3D11_CLEAR_DEPTH);

		pDepthBuffer[2]->SetTargets();

		pShadowPixelShader->Bind();

		// Phase E5 — Drawable shadow iterate / instancing removed (no
		// live Drawable instances). Shadow pass for MeshRenderer Components
		// is a follow-up — currently no Component-side shadow registration
		// exists; reintroduce when the engine wants character shadows on
		// the GameObject path.

		pShadowVertexShader->PostBind();

		pShadowPixelShader->PostBind();

		pDepthBuffer[2]->ResetTargets();
	}

	std::shared_ptr<class Texture> RenderManager::GetHDRDownScaleTexture() const
	{
		return m_pHDRDownScaleTexture;
	}
	float RenderManager::GetFOVValueX() const
	{
		return m_tHDRCBuffer.vDOFFarValues.x;
	}
	float RenderManager::GetFOVValueY()	const
	{
		return m_tHDRCBuffer.vDOFFarValues.y;
	}
	std::shared_ptr<class Texture> RenderManager::GetBloomTexture()	const
	{
		return m_pBloomTexture;
	}
	std::shared_ptr<class Texture> RenderManager::GetBloomFinalTexture()	const
	{
		return m_pBloomFinalTexture;
	}

	void RenderManager::SetFogColor(const Vector3& vColor)
	{
		m_tFogCBuffer.vFogColor = vColor;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);
	}

	void RenderManager::SetFogHighlightColor(const Vector3& vColor)
	{
		m_tFogCBuffer.vFogHighlightColor = vColor;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);
	}

	void RenderManager::SetFogStartDepth(float fDepth)
	{
		m_tFogCBuffer.fFogStartDepth = fDepth;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);
	}

	void RenderManager::SetFogDensity(float fDensity)
	{
		m_tFogCBuffer.fFogGlobalDensity = fDensity;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);
	}

	void RenderManager::SetFogHeightFallOff(float fHeightFallOff)
	{
		m_tFogCBuffer.fFogHeightFallOff = fHeightFallOff;

		m_pFogCBuffer->UpdateBuffer(m_tFogCBuffer);
	}

	const Vector3& RenderManager::GetFogColor() const
	{
		return m_tFogCBuffer.vFogColor;
	}

	const Vector3& RenderManager::GetFogHighlightColor() const
	{
		return m_tFogCBuffer.vFogHighlightColor;
	}

	float RenderManager::GetFogStartDepth() const
	{
		return m_tFogCBuffer.fFogStartDepth;
	}

	float RenderManager::GetFogDensity() const
	{
		return m_tFogCBuffer.fFogGlobalDensity;
	}

	float RenderManager::GetFogHeightFallOff() const
	{
		return m_tFogCBuffer.fFogHeightFallOff;
	}

	void RenderManager::RenderDecal()
	{
		pMRT->SetDepthSRV(10);

		m_pDecalMRT->SetTargets();

		m_pDecalBlend->Bind();

		m_pNoDepthRead->Bind();

		// Phase E5 — Drawable iterate / instancing removed (no live
		// Drawable instances). Component-side decal pass:
		for (const auto& pDecal : m_DecalList)
		{
			if (pDecal) pDecal->Bind();
		}

		m_pNoDepthRead->PostBind();

		m_pDecalBlend->PostBind();

		m_pDecalMRT->ResetTargets();

		pMRT->ResetSRV(10);
	}

	void RenderManager::RenderSkyBox()
	{
		if (!m_pSkyBox)
		{
			return;
		}
		Graphics::GetInst()->ResetBindCache();

		pMRT->SetDepthSRV(10);

		m_pNoDepthRead->Bind();

		m_pCullFront->Bind();

		// Phase E5 — SkyBox is a Component now; bind owner Transform CB
		// (the WVP matrix the skybox shader expects) before SkyBox::Bind,
		// which mirrors the MeshRendererComponent render path.
		GameObject* pSkyBoxOwner = m_pSkyBox->GetGameObjectOwner();
		std::shared_ptr<Transform> pSkyBoxTr =
			pSkyBoxOwner ? pSkyBoxOwner->GetComponent<Transform>() : nullptr;
		if (pSkyBoxTr) pSkyBoxTr->Bind();

		m_pSkyBox->Bind();

		if (pSkyBoxTr) pSkyBoxTr->PostBind();

		m_pCullFront->PostBind();

		m_pNoDepthRead->PostBind();

		pMRT->ResetSRV(10);
	}

	void RenderManager::PostProcessing()
	{
		m_pDownScaleCBuffer->Bind();

		HDRDownScaleFirst();

		HDRDownScaleSecond();

		Bloom();

		RenderHDR();

		m_pPrevAverageLightBuffer.swap(m_pAverageLightBuffer);
	}

	void RenderManager::Clear()
	{
		m_DecalList.clear();

		for (int i = 0; i < static_cast<int>(RENDER_LAYER::END); ++i)
		{
			m_mapMeshInstance[i].clear();
			m_ParticleList[i].clear();
			m_CustomRenderList[i].clear();
		}

		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
		{
			m_LightList[i].clear();
		}

		// Phase E5 — m_RenderList<Drawable>, m_ShadowList, m_mapInstance,
		// m_mapShadowInstance removed; clearing them is no longer needed.

		m_pHDRTexture->Clear();

		m_pBlurTarget->Clear();
	}

	void RenderManager::HDRDownScaleFirst()
	{
		m_pHDRTexture->SetSRV();

		m_pDownScaleFirstCS->Bind();

		m_pLightBuffer->SetUAV(5);

		m_pHDRDownScaleTexture->SetUAV(6);

		int iPixelCount = Window::GetInst()->GetWidth() * Window::GetInst()->GetHeight();

		m_pDownScaleFirstCS->Dispatch(iPixelCount / 1024 + static_cast<bool>(iPixelCount % 1024));

		m_pHDRDownScaleTexture->ResetUAV(6);

		m_pLightBuffer->ResetUAV(5);

		m_pHDRTexture->ResetSRV();
	}

	void RenderManager::HDRDownScaleSecond()
	{
		m_pDownScaleSecondCS->Bind();

		m_pLightBuffer->SetSRV(41);

		m_pAverageLightBuffer->SetUAV(5);

		m_pPrevAverageLightBuffer->SetSRV(2);

		m_pDownScaleSecondCS->Dispatch();

		m_pPrevAverageLightBuffer->ResetSRV(2);

		m_pAverageLightBuffer->ResetUAV(5);

		m_pLightBuffer->ResetSRV(41);
	}

	void RenderManager::RenderHDR()
	{
		m_pHDRCBuffer->Bind();

		m_pHDRTexture->SetSRV(0, 0);

		m_pBloomFinalTexture->Bind();

		m_pHDRDownScaleTexture->Bind();

		pMultiVertexShader->Bind();

		pMRT->SetDepthSRV(10);

		m_pHDRPS->Bind();

		m_pAverageLightBuffer->SetSRV(1);

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

		m_pAverageLightBuffer->ResetSRV(1);

		pMRT->ResetSRV(10);

		m_pHDRDownScaleTexture->ResetSRV();

		m_pHDRTexture->ResetSRV(0);

		m_pBloomFinalTexture->ResetSRV();
	}

	void RenderManager::Bloom()
	{
		Bright();

		BloomFilter();
	}

	void RenderManager::Bright()
	{
		m_pBloomTexture->SetUAV(0);

		m_pHDRDownScaleTexture->Bind();

		m_pAverageLightBuffer->SetSRV(1);

		m_pBrightCS->Bind();

		m_pBrightCS->Dispatch(m_tDownScaleCBuffer.iGroupSize);

		m_pAverageLightBuffer->ResetSRV(1);

		m_pHDRDownScaleTexture->ResetSRV();

		m_pBloomTexture->ResetUAV(0);
	}
	void RenderManager::RenderBlur()
	{
		m_pBlurTarget->SetTargets(pMRT->GetDSV());

		m_pDestAlpha->Bind();

		pDepthBuffer[static_cast<int>(LIGHT_TYPE::DIRECTIONAL)]->SetDepthSRV(15);

		// Phase E5 — Drawable iterate removed (no live instances).
		// Component-side particle pass for BLUR layer.
		for (const auto& pParticle : m_ParticleList[static_cast<int>(RENDER_LAYER::BLUR)])
		{
			if (pParticle) pParticle->Bind();
		}

		// Phase E5 — generic Component render callbacks for BLUR layer
		// (used by Client::Trail and any future custom Component).
		for (const auto& fn : m_CustomRenderList[static_cast<int>(RENDER_LAYER::BLUR)])
		{
			if (fn) fn();
		}

		pDepthBuffer[static_cast<int>(LIGHT_TYPE::DIRECTIONAL)]->ResetSRV(15);

		m_pDestAlpha->PostBind();

		m_pBlurTarget->ResetTargets();

		m_pBlurTarget->SetSRV(0, 0);

		m_pBlurTexture->SetUAV(0);

		m_pDownScaleCBuffer->Bind();

		m_pBlurCS->Bind();

		m_pBlurCS->Dispatch(static_cast<int>(ceil((Window::GetInst()->GetWidth() * Window::GetInst()->GetHeight()) / 1024.f)));

		m_pBlurTexture->ResetUAV(0);

		m_pBlurTarget->ResetSRV(0);

		m_pAlphaBlend->Bind();

		m_pBlurTexture->Bind();

		pBlurNullVertexShader->Bind();

		pBlurNullPixelShader->Bind();

		m_pHDRTexture->SetTargets();

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

		m_pBlurTexture->PostBind();

		m_pHDRTexture->ResetTargets();

		m_pAlphaBlend->PostBind();
	}
#ifdef _DEBUG
	void RenderManager::RenderDebug()
	{
		m_pNoDepthRead->Bind();

		pNullVertexShader->Bind();

		pNullPixelShader->Bind();

		Graphics::GetInst()->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		Vector3 vPos = {100.f, 100.f, 0.f};

		TRANSFORMBUFFER tBuffer = {};

		for (int i = 0; i < 4; ++i)
		{
			pMRT->SetSRV(i, 0);

			tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
				Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

			tBuffer.matWorldViewProject.Transpose();

			m_pTransformBuffer->UpdateBuffer(tBuffer);

			m_pTransformBuffer->Bind();

			Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

			vPos.y += 100.f;
		}

		vPos.x += 100.f;
		vPos.y = 100.f;

		pMRT->SetDepthSRV(0);

		tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
			Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

		tBuffer.matWorldViewProject.Transpose();

		m_pTransformBuffer->UpdateBuffer(tBuffer);

		m_pTransformBuffer->Bind();

		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

		vPos.y += 100.f;

		for (int i = 0; i < 3; ++i)
		{
			pDepthBuffer[i]->SetDepthSRV(0);

			tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
				Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

			tBuffer.matWorldViewProject.Transpose();

			m_pTransformBuffer->UpdateBuffer(tBuffer);

			m_pTransformBuffer->Bind();

			Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

			vPos.y += 100.f;
		}

		vPos.x += 100.f;
		vPos.y = 100.f;

		for (int i = 0; i < 4; ++i)
		{
			m_pDecalMRT->SetSRV(i, 0);

			tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
				Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

			tBuffer.matWorldViewProject.Transpose();

			m_pTransformBuffer->UpdateBuffer(tBuffer);

			m_pTransformBuffer->Bind();

			Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

			vPos.y += 100.f;
		}

		vPos.x += 100.f;
		vPos.y = 100.f;

		m_pHDRTexture->SetSRV(0, 0);

		tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
			Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

		tBuffer.matWorldViewProject.Transpose();

		m_pTransformBuffer->UpdateBuffer(tBuffer);

		m_pTransformBuffer->Bind();

		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

		vPos.y += 100.f;

		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(0, 1, m_pBloomFinalTexture->GetSRV().GetAddressof());

		tBuffer.matWorldViewProject = Matrix::Scaling(100.f, 100.f, 1.f) * Matrix::TranslateFromVector(vPos) *
			Matrix::OthorGraphicLH(0.f, static_cast<float>(Window::GetInst()->GetWidth()), static_cast<float>(Window::GetInst()->GetHeight()), 0.f, 0.f, 500.f);

		tBuffer.matWorldViewProject.Transpose();

		m_pTransformBuffer->UpdateBuffer(tBuffer);

		m_pTransformBuffer->Bind();

		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);

		pMRT->ResetSRV(0);

		m_pNoDepthRead->PostBind();
	}
#endif
	void RenderManager::RenderUI()
	{
		Graphics::GetInst()->ResetBindCache();

		m_pAlphaBlend->Bind();

		// Phase E5 — Drawable iterate / instancing removed (no live
		// Drawable instances). Future UI Component-side render pass
		// would hook in here (none active yet — UI hierarchy is dead).

		// Generic Component render callbacks for UI layer (e.g., editor
		// selection-outline pass drawn on top of the final back-buffer image).
		for (const auto& fn : m_CustomRenderList[static_cast<int>(RENDER_LAYER::UI)])
		{
			if (fn) fn();
		}

		m_pAlphaBlend->PostBind();
	}

	void RenderManager::BloomFilter()
	{
		m_pBloomTexture->Bind();

		m_pBloomFinalTexture->SetUAV(0);

		m_pBloomVerticalFilterCS->Bind();

		m_pBloomVerticalFilterCS->Dispatch(m_tDownScaleCBuffer.iResX, static_cast<int>(ceil(m_tDownScaleCBuffer.iResY / (128.f - 12.f))));

		m_pBloomHorizontalFilterCS->Bind();

		m_pBloomHorizontalFilterCS->Dispatch(static_cast<int>(ceil(m_tDownScaleCBuffer.iResX / (128.f - 12.f))), m_tDownScaleCBuffer.iResY);

		m_pBloomFinalTexture->ResetUAV(0);

		m_pBloomTexture->ResetSRV();
	}
}
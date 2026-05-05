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
#include "RenderInstancing.h"
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
#include "../RenderV2/RenderQueue.h"
#include "../RenderV2/D3D11Context.h"
#include "../RenderV2/Demo.h"
#include "../RenderV2/GpuResources.h"
#include "../RenderV2/EngineShaderCB.h"
#include <algorithm>

namespace Engine
{
	RenderManager* RenderManager::m_pInst = nullptr;

	RenderManager::RenderManager()
	{
	}

	RenderManager::~RenderManager()
	{
		// Tear down V2 demo state while D3D11 device is still alive. Window's
		// shutdown chain destroys RenderManager before Graphics, so this is
		// the right hook for releasing GpuResources held by long-lived demo
		// statics. Without this they outlive Graphics and leak as Refcount: 1
		// in the D3D11 debug layer report.
		RenderV2::ShutdownDemo();
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

	void RenderManager::SetHDRWhiteSqr(float fWhiteSqr)
	{
		m_tHDRCBuffer.fLumWhiteSqr = fWhiteSqr;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::AddDrawable(const std::shared_ptr<Drawable>& pDrawable)
	{
		size_t iKey = pDrawable->GetInstanceKey();

		const std::shared_ptr<VertexShader>& pVertexShader = pDrawable->GetVertexShader();

		const std::shared_ptr<PixelShader>& pPixelShader = pDrawable->GetPixelShader();

		const std::shared_ptr<Mesh>& pMesh = pDrawable->GetMesh();

		int iLayer = static_cast<int>(pDrawable->GetRenderLayer());

		if (pVertexShader && pPixelShader)
		{
			const std::vector<std::shared_ptr<Texture>>& vecTexture = pDrawable->GetTextures();

			if (pDrawable->IsInViewFrustum())
			{
				if (pDrawable->UseInstance())
				{
					assert(iLayer >= 0 && iLayer < static_cast<int>(RENDER_LAYER::END));
					std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[iLayer].find(iKey);

					if (iter == m_mapInstance[iLayer].end())
					{
						const std::shared_ptr<InputLayout>& pInputLayout = pDrawable->FindChild<InputLayout>();

						std::shared_ptr<InputLayout> pInstInputLayout = pVertexShader->GetInstInputLayout() ? pVertexShader->GetInstInputLayout() : (StaticFindBindable<InputLayout>((pInputLayout ? pInputLayout->GetTag() : std::string()) + "_Inst"));

						if (pInstInputLayout)
						{
							const std::shared_ptr<VertexShader>& pInstVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "Inst");

							const std::shared_ptr<VertexShader>& pInstShadowVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "InstShadow");

							const std::shared_ptr<PixelShader>& pInstPixelShader = StaticFindBindable<PixelShader>(pPixelShader->GetTag() + "Inst");

							std::shared_ptr<RenderInstancing> pRenderInstancing = std::make_shared<RenderInstancing>(pMesh, pInstInputLayout,
								pInstVertexShader, pInstShadowVertexShader, pInstPixelShader, pInstInputLayout ? pInstInputLayout->GetInstSize() : 0, vecTexture);

							pRenderInstancing->SetTag((pMesh ? pMesh->GetTag() : std::string()) +
								pVertexShader->GetTag() + pPixelShader->GetTag());

							pRenderInstancing->AddDrawable(pDrawable);

							std::shared_ptr<RasterizerState> pRasterizerState = std::static_pointer_cast<RasterizerState>(pDrawable->FindChild(BINDABLE_TYPE::RASTERIZER_STATE));

							if (pRasterizerState)
							{
								pRenderInstancing->SetRasterizerState(pRasterizerState);
							}

							std::shared_ptr<Animation> pAnimation = pDrawable->GetAnimation();

							if (pAnimation)
							{
								pRenderInstancing->CreateBoneBuffer(pAnimation->GetSequences());

								pRenderInstancing->SetSkeleton(pAnimation->GetSkeleton());
							}

							m_mapInstance[iLayer].insert(std::make_pair(iKey, pRenderInstancing));
						}
						else
						{
							m_RenderList[iLayer].push_back(pDrawable);
						}
					}
					else
					{
						iter->second->AddDrawable(pDrawable);
					}
				}
				else
				{
					m_RenderList[iLayer].push_back(pDrawable);
				}
			}

			if (pDrawable->IsInLightViewfFrustum())
			{
				std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapShadowInstance.find(iKey);

				if (iter == m_mapShadowInstance.end())
				{
					const std::shared_ptr<InputLayout>& pInputLayout = pDrawable->FindChild<InputLayout>();

					const std::shared_ptr<InputLayout>& pInstInputLayout = pVertexShader->GetInstInputLayout() ? pVertexShader->GetInstInputLayout() : (StaticFindBindable<InputLayout>((pInputLayout ? pInputLayout->GetTag() : std::string()) + "_Inst"));

					const std::shared_ptr<VertexShader>& pInstVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "Inst");

					const std::shared_ptr<VertexShader>& pInstShadowVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "InstShadow");

					const std::shared_ptr<PixelShader>& pInstPixelShader = StaticFindBindable<PixelShader>(pPixelShader->GetTag() + "Inst");

					const std::vector<std::shared_ptr<Texture>>& vecTexture = pDrawable->GetTextures();

					if (pInstInputLayout)
					{
						std::shared_ptr<RenderInstancing> pRenderInstancing = std::make_shared<RenderInstancing>(pMesh, pInstInputLayout,
							pInstVertexShader, pInstShadowVertexShader, pInstPixelShader, pInstInputLayout->GetInstSize(), vecTexture);

						pRenderInstancing->AddDrawable(pDrawable);

						std::shared_ptr<Animation> pAnimation = pDrawable->GetAnimation();

						if (pAnimation)
						{
							pRenderInstancing->SetSkeleton(pAnimation->GetSkeleton());

							pRenderInstancing->CreateBoneBuffer(pAnimation->GetSequences());
						}

						m_mapShadowInstance.insert(std::make_pair(iKey, pRenderInstancing));
					}
				}
				else
				{
					iter->second->AddDrawable(pDrawable);
				}
			}
		}
		else
		{
			if (pDrawable->IsInViewFrustum())
			{
				m_RenderList[iLayer].push_back(pDrawable);
			}

			if (pDrawable->IsInLightViewfFrustum())
			{
				m_ShadowList.push_back(pDrawable);
			}
		}
	}

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

	bool RenderManager::Init()
	{
		Graphics* gfx = Graphics::GetInst();
		m_pV2Queue = std::make_unique<RenderV2::RenderQueue>();
		m_pV2Ctx   = std::make_unique<RenderV2::D3D11Context>(gfx->GetDevice(), gfx->GetDeviceContext());

		// Light CB for V2 lit drawables (b1, mirrors shared.hlsl `light`).
		// Hardcoded directional light for the pilot — overridable by future
		// V2 light registration once a real light system exists.
		{
			auto cb = std::make_shared<RenderV2::ConstantBufferRes>();
			cb->Create(gfx->GetDevice(), sizeof(RenderV2::EngineLightCB));

			RenderV2::EngineLightCB light = {};
			light.vLightPos          = {0.0f, 5.0f, -3.0f};
			light.fConstAttenuation  = 1.0f;
			light.vLightColor        = {1.0f, 1.0f, 1.0f, 1.0f};
			light.vLightAmbientColor = {0.25f, 0.25f, 0.25f, 1.0f};
			light.vLightDir          = {0.3f, -0.7f, 0.5f};   // pointing down-right
			light.fLinearAttenuation = 0.0f;
			light.fQuadraticAttenuation = 0.0f;
			light.iLightType         = 2;   // DIRECTIONAL_LIGHT
			light.fLightIntensity    = 1.0f;

			ID3D11DeviceContext* ctx = gfx->GetDeviceContext();
			D3D11_MAPPED_SUBRESOURCE mapped = {};
			ID3D11Buffer* h = cb->Handle();
			if (SUCCEEDED(ctx->Map(h, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				memcpy(mapped.pData, &light, sizeof(light));
				ctx->Unmap(h, 0);
			}
			m_pV2LightCB = cb;
		}

		const std::vector<DXGI_FORMAT>& format = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };

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
		for (int i = 0; i < static_cast<int>(RENDER_LAYER::END); ++i)
		{
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[i].begin();
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[i].end();

			for (; iter != iterEnd; ++iter)
			{
				if (iter->second->GetCount() < 5)
				{
					const std::list<std::shared_ptr<Drawable>>& RenderList = iter->second->GetRenderList();

					std::list<std::shared_ptr<Drawable>>::const_iterator iterR = RenderList.begin();
					std::list<std::shared_ptr<Drawable>>::const_iterator iterREnd = RenderList.end();

					for (; iterR != iterREnd; ++iterR)
					{
						m_RenderList[i].push_back(*iterR);
					}

					iter->second->Clear();
				}
				else
				{
					iter->second->Update();
				}
			}
		}

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterS = m_mapShadowInstance.begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterSEnd = m_mapShadowInstance.end();

		for (; iterS != iterSEnd; ++iterS)
		{
			if (iterS->second->GetCount() < 5)
			{
				const std::list<std::shared_ptr<Drawable>>& DrawableList = iterS->second->GetRenderList();

				std::list<std::shared_ptr<Drawable>>::const_iterator iterD = DrawableList.begin();
				std::list<std::shared_ptr<Drawable>>::const_iterator iterDEnd = DrawableList.end();

				for (; iterD != iterDEnd; ++iterD)
				{
					m_ShadowList.push_back(*iterD);
				}

				iterS->second->Clear();
			}
			else
			{
				iterS->second->Update();
			}
		}

		m_tDownScaleCBuffer.fAdaptation = fDeltaTime;// std::max(fDeltaTime / 0.016f, 1.f);

		m_pDownScaleCBuffer->UpdateBuffer(m_tDownScaleCBuffer);
	}

	void RenderManager::PreRender()
	{
		for (int i = 0; i < static_cast<int>(RENDER_LAYER::END); ++i)
		{
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[i].begin();
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[i].end();

			for (; iter != iterEnd; ++iter)
			{
				if (iter->second->GetCount() > 4)
				{
					iter->second->PreRender();
				}
			}
		}

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterS = m_mapShadowInstance.begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterSEnd = m_mapShadowInstance.end();

		for (; iterS != iterSEnd; ++iterS)
		{
			if (iterS->second->GetCount() > 4)
			{
				iterS->second->PreRender();
			}
		}
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

		RenderDecal();

		RenderShadow();

		m_pNoDepthWrite->Bind();

		m_pHDRTexture->SetTargets();

		RenderLight();

		RenderSkyBox();

		RenderAlpha();

		m_pHDRTexture->ResetTargets();

		RenderBlur();

		PostProcessing();

		RenderUI();

		m_pNoDepthWrite->PostBind();

#ifdef _DEBUG
		RenderDebug();
#endif

		// RenderV2 flush — draws V2-submitted commands directly to the engine's
		// back buffer + depth buffer after all post-processing/UI. Uses the
		// engine's "Basic" depth preset (LESS_EQUAL, write enabled) so
		// complex meshes occlude themselves correctly. Depth is cleared
		// to 1.0 here because earlier engine passes left it in their own
		// state.
		if (m_pV2Queue && m_pV2Ctx && m_pV2Queue->Size() > 0)
		{
			Graphics* gfx = Graphics::GetInst();
			ID3D11DeviceContext* d3dCtx = gfx->GetDeviceContext();
			ID3D11RenderTargetView* rtv = gfx->GetRTV().Get();
			ID3D11DepthStencilView* dsv = gfx->GetDSV().Get();

			d3dCtx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
			d3dCtx->OMSetRenderTargets(1, &rtv, dsv);

			std::shared_ptr<DepthStencilState> dsBasic =
				StaticFindBindable<DepthStencilState>("Basic");
			if (dsBasic) dsBasic->Bind();

			// Light CB at PS b1 (engine shader convention). One per frame —
			// all V2 lit drawables share it. Bound by RenderManager rather
			// than per-DrawCommand so primitives don't pay for it.
			if (m_pV2LightCB)
			{
				ID3D11Buffer* lightBuf =
					static_cast<RenderV2::ConstantBufferRes*>(m_pV2LightCB.get())->Handle();
				d3dCtx->PSSetConstantBuffers(1, 1, &lightBuf);
			}

			m_pV2Queue->Flush(*m_pV2Ctx);
			m_pV2Queue->Clear();

			if (dsBasic) dsBasic->PostBind();

			// Unbind V2 vertex buffer at slot 0. Some engine fullscreen
			// passes (HDR/Bloom/UI null-VS) issue Draw() without setting
			// their own VB, inheriting whatever was last bound. Leaving
			// V2's small VB attached triggers DEVICE_DRAW_VERTEX_BUFFER_TOO_SMALL.
			ID3D11Buffer* nullVB = nullptr;
			UINT stride = 0, offset = 0;
			d3dCtx->IASetVertexBuffers(0, 1, &nullVB, &stride, &offset);
		}

		Clear();
	}

	void RenderManager::SetBloomThreshold(float fThreshold)
	{
		m_tDownScaleCBuffer.fBloomThreshold = fThreshold;

		m_pDownScaleCBuffer->UpdateBuffer(m_tDownScaleCBuffer);
	}

	void RenderManager::RenderOpaque()
	{
		pMRT->SetTargets();

		// Invalidate the per-shader bound caches at pass entry — between
		// passes the actual GPU state is whatever the previous pass left,
		// which our trackers don't observe.
		VertexShader::ResetBoundCache();
		PixelShader::ResetBoundCache();
		InputLayout::ResetBoundCache();
		Topology::ResetBoundCache();
		Sampler::ResetBoundCache();
		Texture::ResetBoundCache();

		// Sort-by-state: gather opaque drawables, order by VS / PS / Material
		// pointer so adjacent draws share GPU state. Bind/PostBind machinery
		// itself unchanged — this just improves call-order coherence.
		// Alpha pass deliberately untouched (back-to-front depth order
		// matters there).
		std::vector<Drawable*> sorted;
		sorted.reserve(m_RenderList[0].size());
		for (const auto& d : m_RenderList[0])
			sorted.push_back(d.get());

		std::sort(sorted.begin(), sorted.end(), [](Drawable* a, Drawable* b)
		{
			const auto* avs = a->GetVertexShader().get();
			const auto* bvs = b->GetVertexShader().get();
			if (avs != bvs) return avs < bvs;
			const auto* aps = a->GetPixelShader().get();
			const auto* bps = b->GetPixelShader().get();
			if (aps != bps) return aps < bps;
			return a->GetMaterial().get() < b->GetMaterial().get();
		});

		for (Drawable* d : sorted)
			d->Bind();

		RenderOpaqueInst();

		pMRT->ResetTargets();
	}

	void RenderManager::SetFOVValueX(float fX)
	{
		m_tHDRCBuffer.vDOFFarValues.x = fX;

		m_pHDRCBuffer->UpdateBuffer(m_tHDRCBuffer);
	}

	void RenderManager::RenderOpaqueInst()
	{
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[0].begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[0].end();

		for (; iter != iterEnd; ++iter)
		{
			if (iter->second->GetCount())
			{
				iter->second->Render();
			}
		}
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
		VertexShader::ResetBoundCache();
		PixelShader::ResetBoundCache();
		InputLayout::ResetBoundCache();
		Topology::ResetBoundCache();
		Sampler::ResetBoundCache();
		Texture::ResetBoundCache();

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

				std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList[static_cast<int>(RENDER_LAYER::ALPHA)].begin();
				std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList[static_cast<int>(RENDER_LAYER::ALPHA)].end();

				for (; iter != iterEnd; ++iter)
				{
					(*iter)->Bind();
				}

				std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterI = m_mapInstance[static_cast<int>(RENDER_LAYER::ALPHA)].begin();
				std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterIEnd = m_mapInstance[static_cast<int>(RENDER_LAYER::ALPHA)].end();

				for (; iterI != iterIEnd; ++iterI)
				{
					if (iterI->second->GetCount())
					{
						iterI->second->Render();
					}
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

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_ShadowList.begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_ShadowList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetAnimation())
			{
				pAnimShadowVertexShader->Bind();
			}
			else
			{
				pShadowVertexShader->Bind();
			}

			(*iter)->DrawShadow();
		}

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterI = m_mapShadowInstance.begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterIEnd = m_mapShadowInstance.end();

		for (; iterI != iterIEnd; ++iterI)
		{
			if (!iterI->second->GetCount())
			{
				continue;
			}

			iterI->second->RenderShadow();
		}

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

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList[static_cast<int>(RENDER_LAYER::DECAL)].begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList[static_cast<int>(RENDER_LAYER::DECAL)].end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Bind();
		}

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterI = m_mapInstance[static_cast<int>(RENDER_LAYER::DECAL)].begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterIEnd = m_mapInstance[static_cast<int>(RENDER_LAYER::DECAL)].end();

		for (; iterI != iterIEnd; ++iterI)
		{
			if (iterI->second->GetCount() > 0)
			{
				iterI->second->Render();
			}
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
		VertexShader::ResetBoundCache();
		PixelShader::ResetBoundCache();
		InputLayout::ResetBoundCache();
		Topology::ResetBoundCache();
		Sampler::ResetBoundCache();
		Texture::ResetBoundCache();

		pMRT->SetDepthSRV(10);

		m_pNoDepthRead->Bind();

		m_pCullFront->Bind();

		m_pSkyBox->Bind();

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
		for (int i = 0; i < static_cast<int>(RENDER_LAYER::END); ++i)
		{
			m_RenderList[i].clear();

			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterR = m_mapInstance[i].begin();
			std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterREnd = m_mapInstance[i].end();

			for (; iterR != iterREnd; ++iterR)
			{
				iterR->second->Clear();
			}
		}

		for (int i = 0; i < static_cast<int>(LIGHT_TYPE::END); ++i)
		{
			m_LightList[i].clear();
		}

		m_ShadowList.clear();

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterS = m_mapShadowInstance.begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterSEnd = m_mapShadowInstance.end();

		for (; iterS != iterSEnd; ++iterS)
		{
			iterS->second->Clear();
		}

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

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList[static_cast<int>(RENDER_LAYER::BLUR)].begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList[static_cast<int>(RENDER_LAYER::BLUR)].end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Bind();
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
		VertexShader::ResetBoundCache();
		PixelShader::ResetBoundCache();
		InputLayout::ResetBoundCache();
		Topology::ResetBoundCache();
		Sampler::ResetBoundCache();
		Texture::ResetBoundCache();

		m_pAlphaBlend->Bind();

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList[static_cast<int>(RENDER_LAYER::UI)].begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList[static_cast<int>(RENDER_LAYER::UI)].end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Bind();
		}

		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterI = m_mapInstance[static_cast<int>(RENDER_LAYER::UI)].begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterIEnd = m_mapInstance[static_cast<int>(RENDER_LAYER::UI)].end();

		for (; iterI != iterIEnd; ++iterI)
		{
			if (iterI->second->GetCount())
			{
				iterI->second->Render();
			}
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
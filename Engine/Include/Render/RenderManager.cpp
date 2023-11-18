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
#include "../Bindable/TransformBuffer.h"
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

namespace Engine
{
	RenderManager* RenderManager::m_pInst = nullptr;

	RenderManager::RenderManager()
	{
	}

	RenderManager::~RenderManager()
	{
	}

	void RenderManager::SetSkyBox(std::shared_ptr<SkyBox> pSkyBox)
	{
		m_pSkyBox = pSkyBox;
	}

	void RenderManager::AddLight(const std::shared_ptr<PointLight>& pLight)
	{
		m_LightList[static_cast<int>(pLight->GetLightType())].push_back(pLight);
	}

	void RenderManager::AddDrawable(const std::shared_ptr<Drawable>& pDrawable)
	{
		const std::shared_ptr<Material>& pMaterial = pDrawable->GetMaterial();

		std::hash<std::string> hs;

		size_t iKey = 1;

		if (pMaterial)
		{
			iKey *= hs(pMaterial->GetTag());
		}

		const std::shared_ptr<VertexShader>& pVertexShader = pDrawable->GetVertexShader();

		const std::shared_ptr<PixelShader>& pPixelShader = pDrawable->GetPixelShader();

		const std::shared_ptr<Mesh>& pMesh = pDrawable->GetMesh();

		int iLayer = static_cast<int>(pDrawable->GetRenderLayer());

		if (pVertexShader && pPixelShader)
		{
			iKey *= pMesh ? hs(pMesh->GetTag()) : 1;

			std::shared_ptr<Animation> pAnimation = pDrawable->GetAnimation();

			iKey *= pAnimation ? hs(pAnimation->GetTag()) : 1;

			if (pDrawable->IsInViewFrustum())
			{
				if (pDrawable->UseInstance())
				{
					std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[iLayer].find(iKey);

					if (iter == m_mapInstance[iLayer].end())
					{
						const std::shared_ptr<InputLayout>& pInputLayout = pDrawable->FindChild<InputLayout>();

						const std::shared_ptr<InputLayout>& pInstInputLayout = pInputLayout ? StaticFindBindable<InputLayout>(pInputLayout->GetTag() + "_Inst") : nullptr;

						const std::shared_ptr<VertexShader>& pInstVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "Inst");

						const std::shared_ptr<VertexShader>& pInstShadowVertexShader = StaticFindBindable<VertexShader>(pVertexShader->GetTag() + "InstShadow");

						const std::shared_ptr<PixelShader>& pInstPixelShader = StaticFindBindable<PixelShader>(pPixelShader->GetTag() + "Inst");

						const std::vector<std::shared_ptr<Texture>>& vecTexture = pDrawable->GetTextures();

						std::shared_ptr<RenderInstancing> pRenderInstancing = std::make_shared<RenderInstancing>(pMesh, pInstInputLayout,
							pInstVertexShader, pInstShadowVertexShader, pInstPixelShader, pInstInputLayout ? pInstInputLayout->GetInstSize() : 0, vecTexture);

						pRenderInstancing->SetTag((pMesh ? pMesh->GetTag() : std::string()) +
							pVertexShader->GetTag() + pPixelShader->GetTag());

						pRenderInstancing->AddDrawable(pDrawable);

						if (pAnimation)
						{
							pRenderInstancing->CreateBoneBuffer(pAnimation->GetSequences());

							pRenderInstancing->SetSkeleton(pAnimation->GetSkeleton());
						}

						m_mapInstance[iLayer].insert(std::make_pair(iKey, pRenderInstancing));
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

					const std::shared_ptr<InputLayout>& pInstInputLayout = StaticFindBindable<InputLayout>(pInputLayout->GetTag() + "_Inst");

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

	std::shared_ptr<class MRT> RenderManager::GetMRT() const
	{
		return pMRT;
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
		pVertexShader = StaticFindBindable<VertexShader>("NullVS");

		if (pVertexShader == nullptr)
		{
			return false;
		}

		pPixelShader = StaticFindBindable<PixelShader>("NullPS");

		if (pPixelShader == nullptr)
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

		m_pNoDepthWrite = StaticFindBindable<DepthStencilState>("NoDepth");

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
	}

	void RenderManager::PreRender()
	{
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iter = m_mapInstance[0].begin();
		std::unordered_map<size_t, std::shared_ptr<RenderInstancing>>::iterator iterEnd = m_mapInstance[0].end();

		for (; iter != iterEnd; ++iter)
		{
			if (iter->second->GetCount() > 4)
			{
				iter->second->PreRender();
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

	void RenderManager::Render()
	{
		pMRT->Clear();

		m_pDecalMRT->Clear();

		RenderOpaque();

		RenderDecal();

		RenderShadow();

		m_pNoDepthWrite->Bind();

		RenderLight();

		RenderSkyBox();

		RenderAlpha();

		m_pNoDepthWrite->PostBind();

		Clear();
	}

	void RenderManager::RenderOpaque()
	{
		pMRT->SetTargets();

		std::list<std::shared_ptr<Drawable>>::iterator iter = m_RenderList[0].begin();
		std::list<std::shared_ptr<Drawable>>::iterator iterEnd = m_RenderList[0].end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Bind();
		}

		RenderOpaqueInst();

		pMRT->ResetTargets();
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

	void RenderManager::RenderAlpha()
	{
		pMRT->SetDepthSRV(10);
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
			}

			pDepthBuffer[i]->ResetSRV(15);
		}

		m_pAlphaBlend->PostBind();
		pMRT->ResetSRV(10);
	}

	void RenderManager::RenderLight()
	{
		const Matrix& matProj = Graphics::GetInst()->GetProjectMatrix();

		PERSPECTIVEBUFFER buffer = {};

		buffer.vPerspective = { 1.f / matProj.ff[0][0],1.f / matProj.ff[1][1], matProj.ff[2][2], -matProj.ff[3][2] };

		const std::shared_ptr<Camera>& pCamera = Graphics::GetInst()->GetCamera();

		if (pCamera)
		{
			const std::shared_ptr<Transform>& pTransform = pCamera->GetTransform();

			const std::shared_ptr<PointLight>& pLight = Graphics::GetInst()->GetLight();

			if (pTransform && pLight)
			{
				buffer.matCameraViewToLightClip = pTransform->GetRotationTranslationMatrix() * pLight->GetViewProject();

				buffer.matCameraViewToLightClip.Transpose();
			}
		}

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

		pMRT->SetDepthSRV(10);

		m_pNoDepthRead->Bind();

		m_pCullFront->Bind();

		m_pSkyBox->Bind();

		m_pCullFront->PostBind();

		m_pNoDepthRead->PostBind();

		pMRT->ResetSRV(10);
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
	}
}
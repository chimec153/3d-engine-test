#include "UIRenderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "Animation.h"
#include "PointLight.h"
#include "Camera.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Texture.h"
#include "Topology.h"
#include "../Core/Graphics.h"
#include "../Render/RenderManager.h"

namespace Engine
{
	UIRenderer::UIRenderer() :
		m_eRenderLayer(RENDER_LAYER::UI)
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	UIRenderer::UIRenderer(const UIRenderer& other) :
		Component(other)
		, m_pCamera(other.m_pCamera)
		, m_pParentTransform(other.m_pParentTransform)
		, m_pParentMesh(other.m_pParentMesh)
		, m_pParentAnimation(other.m_pParentAnimation)
		, m_pLight(other.m_pLight)
		, m_eRenderLayer(other.m_eRenderLayer)
	{
	}

	void UIRenderer::SetCamera(std::shared_ptr<Camera> pCamera)
	{
		m_pCamera = pCamera;
	}

	void UIRenderer::SetTarget(std::shared_ptr<Transform> pTransform,
	                           std::shared_ptr<Mesh> pMesh,
	                           std::shared_ptr<Animation> pAnimation)
	{
		m_pParentTransform = pTransform;
		m_pParentMesh = pMesh;
		m_pParentAnimation = pAnimation;
	}

	bool UIRenderer::Init()
	{
		if (!__super::Init())
			return false;
		return true;
	}

	void UIRenderer::PreDraw(float fDeltaTime)
	{
		__super::PreDraw(fDeltaTime);

		// Phase E5 — register a generic render callback so RenderManager's
		// UI pass invokes Bind without depending on UIRenderer's type.
		std::weak_ptr<UIRenderer> wpSelf = std::dynamic_pointer_cast<UIRenderer>(shared_from_this());
		RenderManager::GetInst()->AddCustomRender(m_eRenderLayer,
			[wpSelf]()
			{
				if (auto pSelf = wpSelf.lock())
					pSelf->Bind();
			});
	}

	void UIRenderer::Bind()
	{
		// Phase E5 — Drawable::BindChild used to bind the UIRenderer's own
		// Bindable child list (shaders/topology/etc.). The Component-shell
		// migration moved those slots up to UIRenderer itself (SetVS /
		// SetPS / SetTexture / SetTopology) so a Component-only UI
		// element can drive its draw through here without a paired
		// MeshRendererComponent.

		if (m_pCamera)
		{
			m_pCamera->PostUpdate(0.f);
			m_pCamera->Update(0.f);

			if (m_pParentTransform)
			{
				m_pParentTransform->UpdateCameraRelateMatrix(m_pCamera);
				m_pParentTransform->Bind();
			}
		}
		else if (m_pParentTransform)
		{
			m_pParentTransform->Bind();
		}

		if (m_pVS)       m_pVS->Bind();
		if (m_pPS)       m_pPS->Bind();
		if (m_pTexture)  m_pTexture->Bind();
		if (m_pTopology) m_pTopology->Bind();

		if (m_pParentAnimation)
		{
			m_pParentAnimation->SetFinalBuffer();
		}

		if (m_pParentMesh)
		{
			// Clear the input layout: the UI VS pipeline reads only
			// SV_VertexID and never consumes VB inputs. A leftover IL
			// from the previous (opaque) pass would mismatch the bound
			// VB or VS signature; null IL keeps the IA passive while
			// Mesh::Draw still binds a VB.
			Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(nullptr);
			Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;

			m_pParentMesh->Draw();
		}

		// Tear down the shader / texture state we set so a subsequent
		// pass starts from a clean slot 0 / VS / PS. Mirrors the manual
		// cleanup HPBar used to do at the end of its render — moved here
		// once UIRenderer owns the shader binding.
		if (m_pVS || m_pPS || m_pTexture)
		{
			auto* pDC = Graphics::GetInst()->GetDeviceContext();
			ID3D11ShaderResourceView* pNullSRV[1] = { nullptr };
			pDC->PSSetShaderResources(0, 1, pNullSRV);
			pDC->VSSetShaderResources(0, 1, pNullSRV);
			if (m_pVS) pDC->VSSetShader(nullptr, nullptr, 0);
			if (m_pPS) pDC->PSSetShader(nullptr, nullptr, 0);
			Graphics::GetInst()->ResetBindCache();
		}
	}

	std::shared_ptr<Component> UIRenderer::Clone()
	{
		return std::make_shared<UIRenderer>(*this);
	}
}

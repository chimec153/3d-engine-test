#include "UIRenderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "Animation.h"
#include "PointLight.h"
#include "Camera.h"
#include "Drawable.h"
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

	void UIRenderer::SetTarget(std::shared_ptr<Drawable> pTarget)
	{
		if (pTarget)
		{
			m_pParentTransform = pTarget->GetTransform();
			m_pParentMesh = pTarget->GetMesh();
			m_pParentAnimation = pTarget->GetAnimation();
		}
		else
		{
			m_pParentTransform = nullptr;
			m_pParentMesh = nullptr;
			m_pParentAnimation = nullptr;
		}
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
		// Bindable child list (shaders/topology/etc.). Stripped for the
		// Component shell; future re-introduction should host this on a
		// GameObject paired with a MeshRendererComponent providing those
		// bindable slots.

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

		if (m_pParentAnimation)
		{
			m_pParentAnimation->SetFinalBuffer();
		}

		if (m_pParentMesh)
		{
			m_pParentMesh->Draw();
		}
	}

	std::shared_ptr<Component> UIRenderer::Clone()
	{
		return std::make_shared<UIRenderer>(*this);
	}
}

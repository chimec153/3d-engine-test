#include "UIRenderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "Animation.h"
#include "PointLight.h"
#include "Camera.h"

namespace Engine
{
	UIRenderer::UIRenderer() :
		Drawable()
	{
		SetRenderLayer(RENDER_LAYER::UI);
		SetBindableType(BINDABLE_TYPE::UIRENDERER);
	}

	void UIRenderer::SetCamera(std::shared_ptr<class Camera> pCamera)
	{
		m_pCamera = pCamera;
	}

	void UIRenderer::SetTarget(std::shared_ptr<class Drawable> pTarget)
	{
		m_pParentTransform = pTarget->GetTransform();

		m_pParentMesh = pTarget->GetMesh();

		m_pParentAnimation = pTarget->GetAnimation();
	}

	bool UIRenderer::Init()
	{
		if (!__super::Init())
		{
			return false;
		}
		return true;
	}

	void UIRenderer::Bind()
	{
		BindChild();

		if (m_pCamera)
		{
			m_pCamera->PostUpdate(0.f);
			m_pCamera->Update(0.f);

			m_pParentTransform->UpdateCameraRelateMatrix(m_pCamera);
		}

		m_pParentTransform->Bind();

		if (m_pParentAnimation)
		{
			m_pParentAnimation->SetFinalBuffer();
		}

		if (m_pParentMesh)
		{
			m_pParentMesh->Draw();
		}
	}
}
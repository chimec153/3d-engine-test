#include "UIControl.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/Transform.h"
#include "../Bindable/UIRenderer.h"
#include "../Bindable/Mesh.h"
#include "../Bindable/Texture.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Topology.h"
#include "../Core/Graphics.h"
#include "../Bindable/InputLayout.h"

namespace Engine
{
	UIControl::UIControl() :
		m_tCBuffer()
	{
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	UIControl::UIControl(const std::string& /*strTexture*/) :
		Component()
		, m_tCBuffer()
		, m_pCBuffer(StaticFindBindable<ConstantBuffer<UICBUFFER>>("UI"))
	{
		// Phase E5 — Drawable-era ctor wired Topology / VS / PS / Texture
		// into the Drawable child list and set the UI render layer.
		// Stripped for the Component shell.
		SetComponentType(COMPONENT_TYPE::NONE);
	}

	UIControl::UIControl(const UIControl& control) :
		Component(control)
		, m_tCBuffer(control.m_tCBuffer)
		, m_pCBuffer(control.m_pCBuffer)
	{
	}

	void UIControl::SetStartUV(const Vector2& vUV)   { m_tCBuffer.vStartUV = vUV; }
	void UIControl::SetEndUV(const Vector2& vUV)     { m_tCBuffer.vEndUV = vUV; }
	void UIControl::SetStartPos(const Vector2& vPos) { m_tCBuffer.vStartPos = vPos; }
	void UIControl::SetSize(const Vector2& vSize)    { m_tCBuffer.vSize = vSize; }

	void UIControl::DrawQuad()
	{
		if (!m_pCBuffer) return;

		m_pCBuffer->UpdateBuffer(m_tCBuffer);
		m_pCBuffer->Bind();

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(nullptr);
		Graphics::GetInst()->GetBindCache().pBoundIL = nullptr;

		Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);
	}

	bool UIControl::Init()
	{
		if (!__super::Init())
			return false;

		// Shared Transform for every UIControl subclass. Lives as a
		// child Component so any UIControl created via CreateComponent
		// on a parent UIControl picks the parent's Transform up
		// automatically (Component::AddChild + Transform::SetParent
		// chain). UI camera mode flags PostUpdate's pixel→NDC path.
		m_pTransform = CreateComponent<Transform>("transform");
		if (m_pTransform)
		{
			m_pTransform->SetCameraType(CAMERA_TYPE::UI);

			// If our owner UIControl already has a Transform child,
			// hook it as our parent so SetRect's pixel values cascade
			// off the ancestor's pixel position.
			if (auto* pParent = GetParent())
			{
				for (const auto& pSibling : pParent->GetChildList())
				{
					if (auto pParentTr =
							std::dynamic_pointer_cast<Transform>(pSibling))
					{
						m_pTransform->SetParentTransform(pParentTr.get());
						break;
					}
				}
			}
		}
		return true;
	}

	void UIControl::SetRect(float fX, float fY, float fW, float fH)
	{
		if (!m_pTransform) return;
		m_pTransform->SetScale   (fW, fH, 1.f);
		m_pTransform->SetPosition(fX, fY, 0.f);
	}

	std::shared_ptr<UIRenderer> UIControl::AddUIRenderer(
		const std::string& strTag,
		const std::shared_ptr<Texture>& pTex,
		const std::shared_ptr<Transform>& pTransform)
	{
		auto pVS       = StaticFindBindable<VertexShader>("UIVS");
		auto pPS       = StaticFindBindable<PixelShader> ("UIPS");
		auto pTopology = StaticFindBindable<Topology>    ("TriangleStrip");
		auto pMesh     = StaticFindBindable<Mesh>        ("UIQuad");
		if (!pVS || !pPS || !pTopology || !pMesh) return nullptr;

		auto pRd = CreateComponent<UIRenderer>(strTag);
		if (!pRd) return nullptr;

		pRd->SetTarget(pTransform ? pTransform : m_pTransform, pMesh, nullptr);
		pRd->SetVertexShader(pVS);
		pRd->SetPixelShader(pPS);
		if (pTex) pRd->SetTexture(pTex);
		pRd->SetTopology(pTopology);
		pRd->SetRenderLayer(RENDER_LAYER::UI);
		return pRd;
	}

	std::shared_ptr<Transform> UIControl::AddQuadTransform(
		const std::string& strTag,
		float fX, float fY, float fW, float fH)
	{
		auto pTr = CreateComponent<Transform>(strTag);
		if (!pTr) return nullptr;
		pTr->SetCameraType(CAMERA_TYPE::UI);
		pTr->SetScale   (fW, fH, 1.f);
		pTr->SetPosition(fX, fY, 0.f);
		return pTr;
	}

	void UIControl::Update(float fDelatTime)
	{
		__super::Update(fDelatTime);
	}

	std::shared_ptr<Component> UIControl::Clone()
	{
		return std::make_shared<UIControl>(*this);
	}
}

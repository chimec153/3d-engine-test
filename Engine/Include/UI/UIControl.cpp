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
#include "../Core/Window.h"
#include "../Input/Input.h"
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
		, m_tAnchor(control.m_tAnchor)
	{
		// m_iResizeToken intentionally NOT copied — the source's token
		// keeps pointing at the source; the clone re-registers in Init.
	}

	UIControl::~UIControl()
	{
		if (m_iResizeToken >= 0 && Window::GetInst())
			Window::GetInst()->UnregisterResizeCallback(m_iResizeToken);
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

		// Lazy lookup of the shared UI cbuffer (b5). The default ctor
		// can't initialise it, so subclasses that go through the
		// default constructor (Text, HPBar, …) still need a handle —
		// fetch it here so any later PushUICBuffer-style code can rely
		// on m_pCBuffer being non-null.
		if (!m_pCBuffer)
			m_pCBuffer = StaticFindBindable<ConstantBuffer<UICBUFFER>>("UI");

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

		// Register for window resize so any anchor spec set later
		// (SetRectByAnchor[Frac]) gets re-resolved automatically.
		// Raw `this` capture is safe — dtor unregisters before destruction.
		if (m_iResizeToken < 0 && Window::GetInst())
		{
			m_iResizeToken = Window::GetInst()->RegisterResizeCallback(
				[this](int, int) { RecomputeRectFromAnchor(); });
		}
		return true;
	}

	void UIControl::SetRect(float fX, float fY, float fW, float fH)
	{
		OnRectChanged(fX, fY, fW, fH);
	}

	void UIControl::OnRectChanged(float fX, float fY, float fW, float fH)
	{
		if (!m_pTransform) return;
		m_pTransform->SetScale   (fW, fH, 1.f);
		m_pTransform->SetPosition(fX, fY, 0.f);
	}

	void UIControl::SetRectByAnchor(Vector2 vAnchor, Vector2 vPivot, Vector2 vSizePx)
	{
		m_tAnchor.vAnchor   = vAnchor;
		m_tAnchor.vPivot    = vPivot;
		m_tAnchor.vSize     = vSizePx;
		m_tAnchor.bSizeFrac = false;
		m_tAnchor.bSet      = true;
		RecomputeRectFromAnchor();
	}

	void UIControl::SetRectByAnchorFrac(Vector2 vAnchor, Vector2 vPivot, Vector2 vSizeFrac)
	{
		m_tAnchor.vAnchor   = vAnchor;
		m_tAnchor.vPivot    = vPivot;
		m_tAnchor.vSize     = vSizeFrac;
		m_tAnchor.bSizeFrac = true;
		m_tAnchor.bSet      = true;
		RecomputeRectFromAnchor();
	}

	void UIControl::RecomputeRectFromAnchor()
	{
		if (!m_tAnchor.bSet) return;
		auto* pWin = Window::GetInst();
		if (!pWin) return;
		const float fSW = static_cast<float>(pWin->GetWidth());
		const float fSH = static_cast<float>(pWin->GetHeight());

		const float fW = m_tAnchor.bSizeFrac ? m_tAnchor.vSize.x * fSW : m_tAnchor.vSize.x;
		const float fH = m_tAnchor.bSizeFrac ? m_tAnchor.vSize.y * fSH : m_tAnchor.vSize.y;
		const float fX = m_tAnchor.vAnchor.x * fSW - m_tAnchor.vPivot.x * fW;
		const float fY = m_tAnchor.vAnchor.y * fSH - m_tAnchor.vPivot.y * fH;
		OnRectChanged(fX, fY, fW, fH);
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

	void UIControl::SetClipRect(float fX, float fY, float fW, float fH)
	{
		std::vector<std::shared_ptr<UIRenderer>> vecRenderers;
		FindChilds<UIRenderer>(vecRenderers);
		for (auto& pRd : vecRenderers)
			if (pRd) pRd->SetClipRect(fX, fY, fW, fH);
	}

	void UIControl::ClearClipRect()
	{
		std::vector<std::shared_ptr<UIRenderer>> vecRenderers;
		FindChilds<UIRenderer>(vecRenderers);
		for (auto& pRd : vecRenderers)
			if (pRd) pRd->ClearClipRect();
	}

	bool UIControl::HitTestMousePx() const
	{
		if (!m_pTransform) return false;
		auto* pInput = CInput::GetInst();
		if (!pInput) return false;
		const float fMx = static_cast<float>(pInput->GetMouseX());
		const float fMy = static_cast<float>(pInput->GetMouseY());
		const Vector3 vPos   = m_pTransform->GetPosition();
		const Vector3 vScale = m_pTransform->GetScale();
		return fMx >= vPos.x && fMx <= vPos.x + vScale.x
			&& fMy >= vPos.y && fMy <= vPos.y + vScale.y;
	}

	void UIControl::Update(float fDelatTime)
	{
		__super::Update(fDelatTime);

		// Mouse-event dispatch. Disabled controls are already skipped by
		// Component::Update before reaching here (the parent's iteration
		// gates on IsEnable). No-op virtuals make the cost a few floats
		// for widgets that don't care about input.
		if (!HitTestMousePx()) return;
		OnHover();
		if (auto* pInput = CInput::GetInst())
		{
			if (pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT))
				OnMouseDown();
			if (pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::RIGHT))
				OnRightMouseDown();
		}
	}

	std::shared_ptr<Component> UIControl::Clone()
	{
		return std::make_shared<UIControl>(*this);
	}
}

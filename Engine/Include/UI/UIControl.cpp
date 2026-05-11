#include "UIControl.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/ConstantBuffer.h"
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
		return true;
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

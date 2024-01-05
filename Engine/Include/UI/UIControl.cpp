#include "UIControl.h"
#include "../Bindable/Topology.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/ConstantBuffer.h"
#include "../Bindable/Transform.h"

Engine::UIControl::UIControl(const std::string& strTexture)	:
	Drawable()
	, m_tCBuffer()
	, m_pCBuffer(FindAndAddBind<ConstantBuffer<UICBUFFER>>("UI"))
{
	SetRenderLayer(RENDER_LAYER::UI);

	FindAndAddBind<Topology>("TriangleStrip");
	FindAndAddBind<VertexShader>("UIVS");
	FindAndAddBind<PixelShader>("UIPS");

	FindAndAddBind<Engine::Texture>(strTexture);
}

Engine::UIControl::UIControl(const UIControl& control)	:
	Drawable(control)
	, m_tCBuffer(control.m_tCBuffer)
	, m_pCBuffer(control.m_pCBuffer)
{
}

void Engine::UIControl::SetStartUV(const Vector2& vUV)
{
	m_tCBuffer.vStartUV = vUV;
}

void Engine::UIControl::SetEndUV(const Vector2& vUV)
{
	m_tCBuffer.vEndUV = vUV;
}

void Engine::UIControl::SetStartPos(const Vector2& vPos)
{
	m_tCBuffer.vStartPos = vPos;
}

void Engine::UIControl::SetSize(const Vector2& vSize)
{
	m_tCBuffer.vSize = vSize;
}

void Engine::UIControl::DrawQuad()
{
	m_pCBuffer->UpdateBuffer(m_tCBuffer);

	m_pCBuffer->Bind();

	Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(nullptr);

	Graphics::GetInst()->GetDeviceContext()->Draw(4, 0);
}

bool Engine::UIControl::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	GetTransform()->SetCameraType(CAMERA_TYPE::UI);

	return true;
}

void Engine::UIControl::Update(float fDelatTime)
{
	__super::Update(fDelatTime);
}

void Engine::UIControl::Bind()
{
	BindChild();

	DrawQuad();
}

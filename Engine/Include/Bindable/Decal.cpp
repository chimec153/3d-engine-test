#include "Decal.h"
#include "InputLayout.h"
#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"
#include "Camera.h"
#include "TransformBuffer.h"

Engine::Decal::Decal()	:
	m_pCBuffer(FindAndAddBind<ConstantBuffer<DECALCBUFFER>>("Decal"))
	, m_bFadeStart(false)
{
	SetBindableType(BINDABLE_TYPE::DECAL);
	SetRenderLayer(RENDER_LAYER::DECAL);

	FindAndAddBind<VertexShader>("DecalVS");
	FindAndAddBind<PixelShader>("DecalPS");
	FindAndAddBind<InputLayout>("Standard");

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());
}

void Engine::Decal::SetMaxFadeTime(float fMax)
{
	m_tCBuffer.fMaxFadeTime = fMax;
}

void Engine::Decal::SetFadeStartTime(float fStart)
{
	m_tCBuffer.fFadeStartTime = fStart;
}

void Engine::Decal::StartFade()
{
	m_bFadeStart = true;
}

void Engine::Decal::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (m_bFadeStart)
	{
		m_tCBuffer.fFadeTime += fDeltaTime;

		if (m_tCBuffer.fMaxFadeTime < m_tCBuffer.fFadeTime)
		{
			m_tCBuffer.fFadeTime = m_tCBuffer.fMaxFadeTime;
		}
	}

	std::shared_ptr<Transform> pTransform = GetTransform();

	m_tCBuffer.matInvWorldView = Graphics::GetInst()->GetCamera()->GetInvView() * Matrix::TranslateFromVector(-pTransform->GetPosition()) * pTransform->GetRotationMatrix().Transpose() * Matrix::Scaling(1.f / pTransform->GetScale());

	m_tCBuffer.matInvWorldView.Transpose();
}

void Engine::Decal::Bind()
{
	m_pCBuffer->UpdateBuffer(m_tCBuffer);

	__super::Bind();
}

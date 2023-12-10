#include "Decal.h"
#include "InputLayout.h"
#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"
#include "Camera.h"
#include "TransformBuffer.h"

Engine::Decal::Decal()	:
	m_pCBuffer(StaticFindBindable<ConstantBuffer<DECALCBUFFER>>("Decal"))
	, m_bFadeStart(false)
{
	SetBindableType(BINDABLE_TYPE::DECAL);
	SetRenderLayer(RENDER_LAYER::DECAL);
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

bool Engine::Decal::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	FindAndAddBind<VertexShader>("DecalVS");
	FindAndAddBind<PixelShader>("DecalPS");
	FindAndAddBind<InputLayout>("Standard");

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());

	return true;
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

	m_pCBuffer->Bind();

	__super::Bind();
}

void Engine::Decal::Save(FILE* pFile)
{
	__super::Save(pFile);

	fwrite(&m_tCBuffer.fFadeTime, 4, 1, pFile);
	fwrite(&m_tCBuffer.fMaxFadeTime, 4, 1, pFile);
	fwrite(&m_tCBuffer.fFadeStartTime, 4, 1, pFile);
	fwrite(&m_bFadeStart, 1, 1, pFile);
}

void Engine::Decal::Load(FILE* pFile)
{
	__super::Load(pFile);

	fread(&m_tCBuffer.fFadeTime, 4, 1, pFile);
	fread(&m_tCBuffer.fMaxFadeTime, 4, 1, pFile);
	fread(&m_tCBuffer.fFadeStartTime, 4, 1, pFile);
	fread(&m_bFadeStart, 1, 1, pFile);
}

#include "Decal.h"
#include "InputLayout.h"
#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"
#include "Camera.h"
#include "Transform.h"

Engine::Decal::Decal()	:
	m_pCBuffer(StaticFindBindable<ConstantBuffer<DECALCBUFFER>>("Decal"))
	, m_bFadeStart(false)
{
	m_tCBuffer.fMaxFadeTime = 1.f;

	SetBindableType(BINDABLE_TYPE::DECAL);
	SetRenderLayer(RENDER_LAYER::DECAL);

	NotUseShadow();
}

Engine::Decal::Decal(const Decal& decal)	:
	Drawable(decal)
	, m_tCBuffer(decal.m_tCBuffer)
	, m_pCBuffer(decal.m_pCBuffer)
	, m_bFadeStart(decal.m_bFadeStart)
{
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

void Engine::Decal::GetInstData(char* pData, int iSize) const
{
	std::shared_ptr<Transform> pTransform = GetTransform();

	memcpy_s(pData, iSize, &pTransform->GetBuffer().matWorldViewProject, 64);
	memcpy_s(pData + 64, iSize - 64, &m_tCBuffer.matInvWorldView, 64);

	std::shared_ptr<Material> pMaterial = GetMaterial();

	if (pMaterial)
	{
		const MATERIAL& tMaterial = pMaterial->GetMaterial();

		memcpy_s(pData + 128, iSize - 128, &tMaterial.diffuseColor, 16);
		memcpy_s(pData + 144, iSize - 144, &tMaterial.specularColor, 16);
		memcpy_s(pData + 160, iSize - 160, &tMaterial.emissiveColor, 16);
		memcpy_s(pData + 176, iSize - 176, &tMaterial.vRoughness, 8);
		memcpy_s(pData + 184, iSize - 184, &tMaterial.fFraction, 4);
	}

	memcpy_s(pData + 188, iSize - 188, &m_tCBuffer.fFadeStartTime, 4);
	memcpy_s(pData + 192, iSize - 192, &m_tCBuffer.fMaxFadeTime, 4);
	memcpy_s(pData + 196, iSize - 196, &m_tCBuffer.fFadeTime, 4);
}

bool Engine::Decal::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	FindAndAddBind<VertexShader>(DECAL_VS);
	FindAndAddBind<InputLayout>(STANDARD_INPUT_LAYOUT);

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());

	return true;
}

void Engine::Decal::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);
}

void Engine::Decal::PostUpdate(float fDeltaTime)
{
	__super::PostUpdate(fDeltaTime);

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

std::shared_ptr<Engine::Bindable> Engine::Decal::Clone()
{
	return std::make_shared<Decal>(*this);
}

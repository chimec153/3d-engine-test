#include "PaperBurn.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "Texture.h"

namespace Engine
{
	Engine::PaperBurn::PaperBurn(std::shared_ptr<class Texture> pTexture) :
		Bindable()
		, m_pPaperBurnTexture(pTexture)
		, m_pCBuffer(StaticFindBindable<ConstantBuffer<PAPERBURNCBUFFER>>("PaperBurn"))
		, m_bStart(false)
	{
		SetBindableType(BINDABLE_TYPE::PAPERBURN);
	}

	Engine::PaperBurn::PaperBurn(const PaperBurn& paper) :
		Bindable(paper)
		, m_pPaperBurnTexture(paper.m_pPaperBurnTexture)
		, m_tCBuffer(paper.m_tCBuffer)
		, m_pCBuffer(paper.m_pCBuffer)
		, m_bStart(paper.m_bStart)
	{
	}

	void Engine::PaperBurn::SetPaperBurnTexture(std::shared_ptr<Texture> pTexture)
	{
		m_pPaperBurnTexture = pTexture;
	}

	void Engine::PaperBurn::SetMaxTime(float fMax)
	{
		m_tCBuffer.fMaxTime = fMax;
	}

	void Engine::PaperBurn::StartPaperBurn()
	{
		m_bStart = true;
	}

	void Engine::PaperBurn::SetStartColor(const Vector4& vColor)
	{
		m_tCBuffer.vStartColor = vColor;
	}

	void Engine::PaperBurn::SetMidColor(const Vector4& vColor)
	{
		m_tCBuffer.vMidColor = vColor;
	}

	void Engine::PaperBurn::SetFinalColor(const Vector4& vColor)
	{
		m_tCBuffer.vFinalColor = vColor;
	}

	void Engine::PaperBurn::SetStartRate(float fRate)
	{
		m_tCBuffer.fStartRate = fRate;
	}

	void Engine::PaperBurn::SetMidRate(float fRate)
	{
		m_tCBuffer.fMidRate = fRate;
	}

	void Engine::PaperBurn::SetFinalRate(float fRate)
	{
		m_tCBuffer.fFinalRate = fRate;
	}

	void Engine::PaperBurn::SetEndRate(float fRate)
	{
		m_tCBuffer.fEndRate = fRate;
	}

	void Engine::PaperBurn::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_bStart)
		{
			m_tCBuffer.fTime += fDeltaTime;

			if (m_tCBuffer.fTime >= m_tCBuffer.fMaxTime)
			{
				m_bStart = false;
			}
		}
	}

	void Engine::PaperBurn::Bind()
	{
		m_pPaperBurnTexture->Bind();

		m_pCBuffer->UpdateBuffer(m_tCBuffer);

		m_pCBuffer->Bind();
	}

	std::shared_ptr<Bindable> Engine::PaperBurn::Clone()
	{
		return std::make_shared<PaperBurn>(*this);
	}
}
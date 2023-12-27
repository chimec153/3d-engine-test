#include "PaperBurn.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "Texture.h"

namespace Engine
{
	PaperBurn::PaperBurn()
	{
	}
	Engine::PaperBurn::PaperBurn(std::shared_ptr<class Texture> pTexture) :
		Bindable()
		, m_pPaperBurnTexture(pTexture)
		, m_pCBuffer(StaticFindBindable<ConstantBuffer<PAPERBURNCBUFFER>>("PaperBurn"))
		, m_bStart(false)
		, m_bCalled()
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
		memcpy_s(m_bCalled, static_cast<int>(PAPER_BURN_STAGE::END), paper.m_bCalled, static_cast<int>(PAPER_BURN_STAGE::END));
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

	void PaperBurn::AddCallBack(PAPER_BURN_STAGE eStage, std::function<void(float)> pFunc)
	{
		m_vecCallBack[static_cast<int>(eStage)].push_back(pFunc);
	}

	void PaperBurn::AddCallBack(PAPER_BURN_STAGE eStage, void(*pFunc)(float))
	{
		m_vecCallBack[static_cast<int>(eStage)].push_back(std::bind(pFunc, std::placeholders::_1));
	}

	void Engine::PaperBurn::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_bStart)
		{
			m_tCBuffer.fTime += fDeltaTime;

			if (m_tCBuffer.fTime >= m_tCBuffer.fMaxTime)
			{
				for (int i = 0; i < static_cast<int>(m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::OUT_STAGE)].size()); ++i)
				{
					m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::OUT_STAGE)][i](m_tCBuffer.fTime);
				}

				m_bStart = false;
			}
			else if (!m_bCalled[static_cast<int>(PAPER_BURN_STAGE::FINAL)] && m_tCBuffer.fTime >= m_tCBuffer.fFinalRate)
			{
				m_bCalled[static_cast<int>(PAPER_BURN_STAGE::FINAL)] = true;

				for (int i = 0; i < static_cast<int>(m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::FINAL)].size()); ++i)
				{
					m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::FINAL)][i](m_tCBuffer.fTime);
				}
			}
			else if (!m_bCalled[static_cast<int>(PAPER_BURN_STAGE::MID)] && m_tCBuffer.fTime >= m_tCBuffer.fMidRate)
			{
				m_bCalled[static_cast<int>(PAPER_BURN_STAGE::MID)] = true;

				for (int i = 0; i < static_cast<int>(m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::MID)].size()); ++i)
				{
					m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::MID)][i](m_tCBuffer.fTime);
				}
			}
			else if (!m_bCalled[static_cast<int>(PAPER_BURN_STAGE::START)] && m_tCBuffer.fTime >= m_tCBuffer.fStartRate)
			{
				m_bCalled[static_cast<int>(PAPER_BURN_STAGE::START)] = true;

				for (int i = 0; i < static_cast<int>(m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::START)].size()); ++i)
				{
					m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::START)][i](m_tCBuffer.fTime);
				}
			}
			else if (!m_bCalled[static_cast<int>(PAPER_BURN_STAGE::READY)])
			{
				m_bCalled[static_cast<int>(PAPER_BURN_STAGE::READY)] = true;

				for (int i = 0; i < static_cast<int>(m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::READY)].size()); ++i)
				{
					m_vecCallBack[static_cast<int>(PAPER_BURN_STAGE::READY)][i](m_tCBuffer.fTime);
				}
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
	void PaperBurn::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tCBuffer, sizeof(PAPERBURNCBUFFER), 1, pFile);
		fwrite(&m_bStart, 1, 1, pFile);
	}
	void PaperBurn::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tCBuffer, sizeof(PAPERBURNCBUFFER), 1, pFile);
		fread(&m_bStart, 1, 1, pFile);

		m_pPaperBurnTexture = std::static_pointer_cast<Texture>(FindChild(BINDABLE_TYPE::TEXTURE));
		m_pCBuffer = StaticFindBindable<ConstantBuffer<PAPERBURNCBUFFER>>("PaperBurn");
	}
}
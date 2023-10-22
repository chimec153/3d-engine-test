#include "HullShader.h"

namespace Engine
{
	HullShader::HullShader(const TCHAR* pFilePath, const char* pEntry) :
		Shader(pFilePath, pEntry)
		, m_pShader()
		, m_pPrevShader()
	{
		SetBindableType(BINDABLE_TYPE::HULL_SHADER);

		LoadShader();
	}

	void HullShader::LoadShader()
	{
		if (!LoadShaderFile("hs_5_0"))
		{
			return;
		}

		ID3DBlob* pBlob = GetBlob();

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateHullShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &m_pShader)))
		{
			return;
		}
	}

	void HullShader::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->HSSetShader(*m_pShader, nullptr, 0);
	}

	void HullShader::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->HSSetShader(nullptr, nullptr, 0);
	}

	std::shared_ptr<Bindable> HullShader::Clone()
	{
		return std::shared_ptr<Bindable>();
	}

	void HullShader::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->HSGetShader(&m_pPrevShader, nullptr, 0);

		Graphics::GetInst()->GetDeviceContext()->HSSetShader(*m_pShader, nullptr, 0);
	}

	void HullShader::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->HSSetShader(*m_pPrevShader, nullptr, 0);
	}
}
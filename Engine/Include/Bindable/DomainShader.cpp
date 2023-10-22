#include "DomainShader.h"

namespace Engine
{
	DomainShader::DomainShader(const TCHAR* pFilePath, const char* pEntry) :
		Shader(pFilePath, pEntry)
		, m_pShader()
		, m_pPrevShader()
	{
		SetBindableType(BINDABLE_TYPE::DOMAIN_SHADER);

		LoadShader();
	}

	void DomainShader::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->DSSetShader(*m_pShader, nullptr, 0);
	}

	void DomainShader::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->DSSetShader(nullptr, nullptr, 0);
	}

	std::shared_ptr<Bindable> DomainShader::Clone()
	{
		return std::shared_ptr<Bindable>();
	}

	void DomainShader::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->DSGetShader(&m_pPrevShader, nullptr, 0);

		Graphics::GetInst()->GetDeviceContext()->DSSetShader(*m_pShader, nullptr, 0);
	}

	void DomainShader::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->DSSetShader(*m_pPrevShader, nullptr, 0);
	}

	void DomainShader::LoadShader()
	{
		if (!LoadShaderFile("ds_5_0"))
		{
			return;
		}

		ID3DBlob* pBlob = GetBlob();

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateDomainShader(pBlob->GetBufferPointer(),
			pBlob->GetBufferSize(), nullptr, &m_pShader)))
		{
			return;
		}
	}
}
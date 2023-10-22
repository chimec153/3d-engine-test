#include "PixelShader.h"
#include "../Core/PathManager.h"
#include "../Core/Ptr.h"
#include <d3dcompiler.h>

namespace Engine
{
	PixelShader::PixelShader(const TCHAR* pFilePath, const char* pEntry) :
		Shader(pFilePath, pEntry)
		, pPixelShader()
		, pPrevPixelShader()
	{
		SetBindableType(BINDABLE_TYPE::PIXEL_SHADER);

		LoadShader();
	}

	PixelShader::~PixelShader() noexcept
	{
	}

	void PixelShader::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->PSSetShader(*pPixelShader, nullptr, 0);
	}

	void PixelShader::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->PSGetShader(&pPrevPixelShader, nullptr, 0);

		Graphics::GetInst()->GetDeviceContext()->PSSetShader(*pPixelShader, nullptr, 0);
	}

	void PixelShader::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);
	}

	std::shared_ptr<Bindable> PixelShader::Clone()
	{
		return std::shared_ptr<Bindable>();
	}

	void PixelShader::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->PSSetShader(*pPrevPixelShader, nullptr, 0);
	}

	void PixelShader::Update(float fDeltaTime)
	{
	}

	void PixelShader::LoadShader()
	{
		if (!Shader::LoadShaderFile("ps_5_0"))
		{
			return;
		}

		ID3DBlob* pPixelBlob = GetBlob();

		if (FAILED(Graphics::GetInst()->GetDevice()->CreatePixelShader(pPixelBlob->GetBufferPointer(), pPixelBlob->GetBufferSize(), nullptr, &pPixelShader)))
		{
			assert(false);
			return;
		}
	}
}
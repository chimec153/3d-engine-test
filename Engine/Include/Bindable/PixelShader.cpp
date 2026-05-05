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

	ID3D11PixelShader* PixelShader::s_pBoundPS = nullptr;

	void PixelShader::ResetBoundCache()
	{
		s_pBoundPS = nullptr;
	}

	void PixelShader::Bind()
	{
		ID3D11PixelShader* mine = *pPixelShader;
		if (mine == s_pBoundPS)
			return;
		Graphics::GetInst()->GetDeviceContext()->PSSetShader(mine, nullptr, 0);
		s_pBoundPS = mine;
	}

	void PixelShader::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->PSGetShader(&pPrevPixelShader, nullptr, 0);

		Graphics::GetInst()->GetDeviceContext()->PSSetShader(*pPixelShader, nullptr, 0);
	}

	void PixelShader::PostBind()
	{
		// No-op — see VertexShader::PostBind for rationale.
	}

	std::shared_ptr<Bindable> PixelShader::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
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
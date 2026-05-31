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
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		ID3D11PixelShader* mine = *pPixelShader;
		if (mine == cache.pBoundPS)
			return;
		Graphics::GetInst()->GetDeviceContext()->PSSetShader(mine, nullptr, 0);
		cache.pBoundPS = mine;
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

		CreateFromBlob();
	}

	void PixelShader::CreateFromBlob()
	{
		ID3DBlob* pPixelBlob = GetBlob();
		if (!pPixelBlob) return;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreatePixelShader(pPixelBlob->GetBufferPointer(), pPixelBlob->GetBufferSize(), nullptr, &pPixelShader)))
		{
			assert(false);
			return;
		}
	}
}
#include "VertexShader.h"
#include <d3dcompiler.h>
#include "../Core/Window.h"
namespace Engine
{

	VertexShader::VertexShader(const TCHAR* pFilePath, const char* pEntry) :
		Shader(pFilePath, pEntry)
		, pVertexShader()
		, pPrevVertexShader()
	{
		SetBindableType(BINDABLE_TYPE::VERTEX_SHADER);

		LoadShader();
	}

	VertexShader::~VertexShader() noexcept
	{
	}

	void VertexShader::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShader(*pVertexShader, nullptr, 0);
	}

	void VertexShader::PostBind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShader(nullptr, nullptr, 0);
	}

	std::shared_ptr<Bindable> VertexShader::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}

	void VertexShader::GetAndBind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSGetShader(&pPrevVertexShader, nullptr, 0);

		Graphics::GetInst()->GetDeviceContext()->VSSetShader(*pVertexShader, nullptr, 0);
	}

	void VertexShader::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShader(*pPrevVertexShader, nullptr, 0);
	}

	void VertexShader::Update(float fDeltaTime)
	{
	}

	void VertexShader::LoadShader()
	{
		if (!__super::LoadShaderFile("vs_5_0"))
		{
			return;
		}

		ID3DBlob* pBlob = GetBlob();

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader)))
		{
			assert(false);
			return;
		}
	}
}
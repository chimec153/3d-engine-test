#include "ComputeShader.h"
#include "../Core/Graphics.h"

Engine::ComputeShader::ComputeShader(const TCHAR* pFilePath, const char* pEntry)	:
	Shader(pFilePath, pEntry)
{
	LoadShader();
}

void Engine::ComputeShader::Bind()
{
	Graphics::GetInst()->GetDeviceContext()->CSSetShader(m_pComputeShader.Get(), nullptr, 0);
}

void Engine::ComputeShader::PostBind()
{
	Graphics::GetInst()->GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
}

std::shared_ptr<Engine::Bindable> Engine::ComputeShader::Clone()
{
	return std::shared_ptr<Bindable>();
}

void Engine::ComputeShader::LoadShader()
{
	if (!LoadShaderFile("cs_5_0"))
	{
		return;
	}

	CreateFromBlob();
}

void Engine::ComputeShader::CreateFromBlob()
{
	ID3DBlob* pBlob = GetBlob();
	if (!pBlob) return;

	if (FAILED(Graphics::GetInst()->GetDevice()->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &m_pComputeShader)))
	{
		assert(false);
		return;
	}
}

void Engine::ComputeShader::Dispatch(int x, int y, int z)
{
	Graphics::GetInst()->GetDeviceContext()->Dispatch(x, y, z);
}

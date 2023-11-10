#include "GeometryShader.h"
#include "../Core/Graphics.h"

Engine::GeometryShader::GeometryShader(const TCHAR* pFilePath, const char* pEntry)	:
	Shader(pFilePath, pEntry)
{
	if (!LoadShaderFile("gs_5_0")) 
	{
		assert(false);
	}

	LoadShader();
}

void Engine::GeometryShader::LoadShader()
{
	if (FAILED(Graphics::GetInst()->GetDevice()->CreateGeometryShader(GetBlob()->GetBufferPointer(), GetBlob()->GetBufferSize(), nullptr, &m_pGS))) {
		assert(false);
	}
}

void Engine::GeometryShader::Bind()
{
	Graphics::GetInst()->GetDeviceContext()->GSSetShader(m_pGS.Get(), nullptr, 0);
}

void Engine::GeometryShader::PostBind()
{
	Graphics::GetInst()->GetDeviceContext()->GSSetShader(nullptr, nullptr, 0);
}

std::shared_ptr<Engine::Bindable> Engine::GeometryShader::Clone()
{
	return std::shared_ptr<Bindable>();
}

#include "GeometryShader.h"
#include "../Core/Graphics.h"

Engine::GeometryShader::GeometryShader(const TCHAR* pFilePath, const char* pEntry)	:
	Shader(pFilePath, pEntry)
{
	LoadShader();
}

void Engine::GeometryShader::LoadShader()
{
	if (!LoadShaderFile("gs_5_0"))
	{
		return;
	}

	CreateFromBlob();
}

void Engine::GeometryShader::CreateFromBlob()
{
	ID3DBlob* pBlob = GetBlob();
	if (!pBlob) return;

	if (FAILED(Graphics::GetInst()->GetDevice()->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &m_pGS))) {
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
	return std::static_pointer_cast<Bindable>(shared_from_this());
}

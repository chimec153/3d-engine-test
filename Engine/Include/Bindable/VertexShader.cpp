#include "VertexShader.h"
#include <d3dcompiler.h>
#include "../Core/Window.h"
namespace Engine
{

	VertexShader::VertexShader(const TCHAR* pFilePath, const char* pEntry, std::shared_ptr<class InputLayout> pInputLayout, std::shared_ptr<class InputLayout> pInstInputLayout) :
		Shader(pFilePath, pEntry)
		, pVertexShader()
		, pPrevVertexShader()
		, m_pInputLayout(pInputLayout)
		, m_pInputLayoutInst(pInstInputLayout)
	{
		SetBindableType(BINDABLE_TYPE::VERTEX_SHADER);

		LoadShader();
	}

	VertexShader::~VertexShader() noexcept
	{
	}

	void VertexShader::Bind()
	{
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		ID3D11VertexShader* mine = *pVertexShader;
		if (mine == cache.pBoundVS)
			return;   // sort-by-state win: same shader as last drawable
		Graphics::GetInst()->GetDeviceContext()->VSSetShader(mine, nullptr, 0);
		cache.pBoundVS = mine;
	}

	void VertexShader::PostBind()
	{
		// Intentionally no-op: leave the shader bound for the next drawable
		// to potentially skip the rebind. RenderManager invalidates the
		// cache via ResetBoundCache() at pass boundaries so cross-pass state
		// doesn't leak.
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

	std::shared_ptr<class InputLayout> VertexShader::GetInputLayout() const
	{
		return m_pInputLayout;
	}

	std::shared_ptr<class InputLayout> VertexShader::GetInstInputLayout() const
	{
		return m_pInputLayoutInst;
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
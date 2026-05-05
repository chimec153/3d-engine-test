#include "InputLayout.h"
#include "VertexShader.h"

namespace Engine
{
	InputLayout::InputLayout(const std::shared_ptr<VertexShader>& pShader, D3D11_INPUT_ELEMENT_DESC* pInputElement, int iCount, int iInstSize) :
		Bindable()
		, m_iInstSize(iInstSize)
	{
		SetBindableType(BINDABLE_TYPE::INPUTLAYOUT);

		ID3DBlob* pBlob = pShader->GetBlob();

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateInputLayout(pInputElement, iCount, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &pInputLayout)))
		{
			assert(false);
			return;
		}
	}

	InputLayout::~InputLayout() noexcept
	{
	}

	ID3D11InputLayout* InputLayout::s_pBound = nullptr;

	void InputLayout::ResetBoundCache()
	{
		s_pBound = nullptr;
	}

	void InputLayout::Bind()
	{
		ID3D11InputLayout* mine = *pInputLayout;
		if (mine == s_pBound)
			return;
		Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(mine);
		s_pBound = mine;
	}

	std::shared_ptr<Bindable> InputLayout::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}

	void InputLayout::Update(float fDeltaTime)
	{
	}

	int InputLayout::GetInstSize() const
	{
		return m_iInstSize;
	}
}
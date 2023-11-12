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

	void InputLayout::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->IASetInputLayout(*pInputLayout);
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
#include "IndexBuffer.h"

namespace Engine
{
	IndexBuffer::IndexBuffer(const std::vector<unsigned int>& pData) :
		Bindable()
	{
		SetBindableType(BINDABLE_TYPE::INDEX_BUFFER);

		m_vecData = pData;

		D3D11_BUFFER_DESC tIndexDesc = {};

		tIndexDesc.ByteWidth = 4 * static_cast<int>(m_vecData.size());
		tIndexDesc.Usage = D3D11_USAGE_DEFAULT;
		tIndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		tIndexDesc.StructureByteStride = 4;

		D3D11_SUBRESOURCE_DATA tIndexSub = {};

		tIndexSub.pSysMem = &m_vecData[0];

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&tIndexDesc, &tIndexSub, &pIndexBuffer)))
		{
			return;
		}
	}

	IndexBuffer::~IndexBuffer() noexcept
	{
	}

	void IndexBuffer::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(*pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	}

	std::shared_ptr<Bindable> IndexBuffer::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}

	int IndexBuffer::GetSize() const
	{
		return static_cast<int>(m_vecData.size());
	}

	const std::vector<unsigned int>& IndexBuffer::GetData() const
	{
		return m_vecData;
	}

	void IndexBuffer::Update(float fDeltaTime)
	{
	}
}
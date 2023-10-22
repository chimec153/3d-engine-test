#pragma once
#include "Bindable.h"

namespace Engine
{
	class ENGINE_DLL VertexBuffer :
		public Bindable
	{
		friend class BindableManager<VertexBuffer>;
		friend class Drawable;

	public:
		template <typename T>
		VertexBuffer(const std::vector<T>& vecVertex);
		VertexBuffer(const void* pData, int iCount, int iSize);
		virtual ~VertexBuffer() noexcept override = default;

	private:
		CPtr<ID3D11Buffer> pBuffer;
		int	m_iCount;
		int m_iSize;

	public:
		int GetCount()	const;
		const CPtr<ID3D11Buffer>& GetBuffer()	const;
		int GetSize()	const;

	public:
		virtual void Update(float fDeltaTime) override;
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;
		void CreateBuffer(const void* pData, int iCount);
	};

	inline VertexBuffer::VertexBuffer(const void* pData, int iCount, int iSize) :
		Bindable()
		, m_iCount(iCount)
		, m_iSize(iSize)
	{
		SetBindableType(BINDABLE_TYPE::VERTEX_BUFFER);

		CreateBuffer(pData, iCount);
	}

	inline int VertexBuffer::GetSize() const
	{
		return m_iSize;
	}

	template<typename T>
	inline VertexBuffer::VertexBuffer(const std::vector<T>& vecVertex) :
		Bindable()
		, m_iCount(static_cast<int>(vecVertex.size()))
		, m_iSize(sizeof(T))
	{
		SetBindableType(BINDABLE_TYPE::VERTEX_BUFFER);

		CreateBuffer(&vecVertex[0], static_cast<int>(vecVertex.size()));
	}

	inline void VertexBuffer::Update(float fDeltaTime)
	{
	}

	inline void VertexBuffer::Bind()
	{
		UINT iStride = m_iSize;
		UINT iOffset = 0;

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 1, pBuffer.GetAdressof(), &iStride, &iOffset);
	}

	inline std::shared_ptr<Bindable> VertexBuffer::Clone()
	{
		return std::shared_ptr<Bindable>();
	}

	inline void VertexBuffer::CreateBuffer(const void* pData, int iCount)
	{
		D3D11_BUFFER_DESC desc = {};

		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.StructureByteStride = m_iSize * iCount;
		desc.ByteWidth = m_iSize * iCount;

		D3D11_SUBRESOURCE_DATA tSub = {};

		tSub.pSysMem = pData;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&desc, &tSub, &pBuffer)))
		{
			return;
		}
	}

	inline int VertexBuffer::GetCount()	const
	{
		return m_iCount;
	}

	inline const CPtr<ID3D11Buffer>& VertexBuffer::GetBuffer()	const
	{
		return pBuffer;
	}
}
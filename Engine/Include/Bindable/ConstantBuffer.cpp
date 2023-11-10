#include "ConstantBuffer.h"
#include "../Core/Graphics.h"

namespace Engine
{
	template<typename T>
	inline void ConstantBuffer<T>::SetSlot(int iSlot)
	{
		m_iSlot = iSlot;
	}

	template<typename T>
	inline int ConstantBuffer<T>::GetSlot() const
	{
		return m_iSlot;
	}

	template<typename T>
	inline const CPtr<ID3D11Buffer>& ConstantBuffer<T>::GetBuffer() const
	{
		return pConstantBuffer;
	}

	template<typename T>
	inline CPtr<ID3D11Buffer>& ConstantBuffer<T>::GetPrevBuffer()
	{
		return pPrevConstantBuffer;
	}

	template<typename T>
	inline void ConstantBuffer<T>::SetBuffer(const CPtr<ID3D11Buffer>& pBuffer)
	{
		pConstantBuffer = pBuffer;
	}

	template<typename T>
	inline ConstantBuffer<T>::ConstantBuffer(int iSlot) :
		Bindable()
		, m_iSlot(iSlot)
	{
		CreateBuffer();
	}

	template<typename T>
	inline ConstantBuffer<T>::ConstantBuffer(const T* pData, int iSlot) :
		Bindable()
		, m_iSlot(iSlot)
	{
		CreateBuffer(pData);
	}

	template<typename T>
	inline ConstantBuffer<T>::ConstantBuffer(const ConstantBuffer& buffer) :
		Bindable(buffer)
		, pConstantBuffer(nullptr)
		, m_iSlot(buffer.m_iSlot)
	{
		CreateBuffer();
	}

	template<typename T>
	inline ConstantBuffer<T>::ConstantBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iSlot) :
		pConstantBuffer(pBuffer)
		, m_iSlot(iSlot)
	{
	}

	template<typename T>
	inline void ConstantBuffer<T>::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->HSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->DSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->GSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAdressof());
	}

	template<typename T>
	std::shared_ptr<Bindable> ConstantBuffer<T>::Clone()
	{
		assert(false);
		return nullptr;
	}

	template<typename T>
	inline void ConstantBuffer<T>::UpdateBuffer(const T& pData)
	{
		D3D11_MAPPED_SUBRESOURCE tSub = {};

		Graphics::GetInst()->GetDeviceContext()->Map(*pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &tSub);

		memcpy_s(tSub.pData, sizeof(T), &pData, sizeof(T));

		Graphics::GetInst()->GetDeviceContext()->Unmap(*pConstantBuffer, 0);
	}

	template<typename T>
	inline void ConstantBuffer<T>::CreateBuffer(const T* pData)
	{
		D3D11_BUFFER_DESC desc = {};

		desc.ByteWidth = sizeof(T);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.StructureByteStride = sizeof(T);

		if (pData)
		{
			D3D11_SUBRESOURCE_DATA tSub = {};

			tSub.pSysMem = pData;

			if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&desc, &tSub, &pConstantBuffer)))
			{
				assert(false);
				return;
			}
		}
		else
		{
			if (FAILED(Graphics::GetInst()->GetDevice()->CreateBuffer(&desc, nullptr, &pConstantBuffer)))
			{
				assert(false);
				return;
			}
		}
	}
	template<typename T>
	void ConstantBuffer<T>::GetAndBind()
	{
		std::shared_ptr<ID3D11Buffer>& pPrevBuffer = ConstantBuffer<T>::GetPrevBuffer();

		Graphics::GetInst()->GetDeviceContext()->PSGetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, &pPrevBuffer);

		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAdressof());
	}
	template<typename T>
	void ConstantBuffer<T>::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::GetPrevBuffer().GetAdressof());
	}
}
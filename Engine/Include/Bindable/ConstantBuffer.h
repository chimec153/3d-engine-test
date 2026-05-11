#pragma once
#include "Bindable.h"
#include "../Core/Window.h"
#include "../Core/Graphics.h"

namespace Engine
{
	// No ENGINE_DLL on this template class: with all member definitions
	// inline below, every TU instantiates ConstantBuffer<T> locally rather
	// than importing it from Engine.dll. Marking the class dllimport in
	// consumers would forbid the inline definitions (C2491). Bindable (the
	// base) is still ENGINE_DLL'd as it should be.
	template <typename T>
	class ConstantBuffer :
		public Bindable
	{
	public:
		ConstantBuffer(int iSlot = 0);
		ConstantBuffer(const T* pData, int iSlot = 0);
		ConstantBuffer(const ConstantBuffer& buffer);
		ConstantBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iSlot = 0);
		virtual ~ConstantBuffer() noexcept override = default;

	protected:
		CPtr<ID3D11Buffer> pConstantBuffer;
		CPtr<ID3D11Buffer> pPrevConstantBuffer;

	private:
		int m_iSlot;

	public:
		void SetSlot(int iSlot);
		int GetSlot()	const;
		const CPtr<ID3D11Buffer>& GetBuffer()	const;
		CPtr<ID3D11Buffer>& GetPrevBuffer();
		void SetBuffer(const CPtr<ID3D11Buffer>& pBuffer);

	public:
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;

	public:
		template <typename P>
		void UpdateBuffer(const P& pData, int iOffset = 0)
		{
			UpdateBuffer(static_cast<const void*>(&pData), sizeof(P), iOffset);
		}
		void UpdateBuffer(const void* pData, int iSize, int iOffset);
		void CreateBuffer(const T* pData = nullptr);
		void GetAndBind();
		void BindEnd();
	};

	// Template implementations live in the header so consumers outside
	// Engine.dll (e.g., Editor instantiating ConstantBuffer with editor-
	// local structs) can produce all needed symbols locally rather than
	// relying on Engine.dll's pre-instantiated set.

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
		Graphics::GetInst()->GetDeviceContext()->VSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->HSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->DSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->GSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetConstantBuffers(GetSlot(), 1, pConstantBuffer.GetAddressof());
	}

	template<typename T>
	inline std::shared_ptr<Bindable> ConstantBuffer<T>::Clone()
	{
		assert(false);
		return nullptr;
	}

	template<typename T>
	inline void ConstantBuffer<T>::UpdateBuffer(const void* pData, int iSize, int iOffset)
	{
		D3D11_MAPPED_SUBRESOURCE tSub = {};

		Graphics::GetInst()->GetDeviceContext()->Map(*pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &tSub);

		void* pDest = static_cast<char*>(tSub.pData) + iOffset;

		memcpy_s(pDest, sizeof(T) - iOffset, pData, iSize);

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
	inline void ConstantBuffer<T>::GetAndBind()
	{
		std::shared_ptr<ID3D11Buffer>& pPrevBuffer = ConstantBuffer<T>::GetPrevBuffer();

		Graphics::GetInst()->GetDeviceContext()->PSGetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, &pPrevBuffer);

		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAddressof());
	}

	template<typename T>
	inline void ConstantBuffer<T>::BindEnd()
	{
		Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::GetPrevBuffer().GetAddressof());
	}

}

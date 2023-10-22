#pragma once
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"

namespace Engine
{
	template <typename T>
	class ENGINE_DLL DomainCBuffer :
		public ConstantBuffer<T>
	{
		friend class BindableManager<DomainCBuffer>;
	public:
		DomainCBuffer(int iSlot = 0);
		DomainCBuffer(const T* pData, int iSlot = 0);
		DomainCBuffer(const DomainCBuffer<T>& buffer);
		DomainCBuffer(const class CPtr<ID3D11Buffer>& pBuffer, int iSlot = 0);
		virtual ~DomainCBuffer() noexcept override = default;

	public:
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;
	};

	template<typename T>
	inline DomainCBuffer<T>::DomainCBuffer(int iSlot) :
		ConstantBuffer<T>(iSlot)
	{
	}

	template<typename T>
	inline DomainCBuffer<T>::DomainCBuffer(const T* pData, int iSlot) :
		ConstantBuffer<T>(pData, iSlot)
	{
	}

	template<typename T>
	inline DomainCBuffer<T>::DomainCBuffer(const DomainCBuffer<T>& buffer) :
		ConstantBuffer<T>(buffer)
	{
	}

	template<typename T>
	inline DomainCBuffer<T>::DomainCBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iSlot) :
		ConstantBuffer<T>(pBuffer, iSlot)
	{
	}

	template<typename T>
	inline void DomainCBuffer<T>::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->DSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAdressof());
	}

	template<typename T>
	inline std::shared_ptr<Bindable> DomainCBuffer<T>::Clone()
	{
		return std::make_shared<DomainCBuffer<T>>(*this);
	}
}
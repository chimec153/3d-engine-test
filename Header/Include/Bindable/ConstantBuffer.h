#pragma once
#include "Bindable.h"
#include "../Core/Window.h"

namespace Engine
{
	template <typename T>
	class ENGINE_DLL ConstantBuffer :
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
		void UpdateBuffer(const T& pData);
		void CreateBuffer(const T* pData = nullptr);
		void GetAndBind();
		void BindEnd();
	};

}
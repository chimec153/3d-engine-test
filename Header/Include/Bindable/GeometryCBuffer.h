#pragma once
#include "ConstantBuffer.h"
#include "../Core/Graphics.h"

namespace Engine
{
    template <typename T>
    class ENGINE_DLL GeometryCBuffer :
        public ConstantBuffer<T>
    {
    public:
        GeometryCBuffer(int iSlot);
        GeometryCBuffer(T* pData, int iSlot);
        GeometryCBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iSlot = 0);
        virtual ~GeometryCBuffer() override = default;

    public:
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
    template<typename T>
    inline GeometryCBuffer<T>::GeometryCBuffer(int iSlot)   :
        ConstantBuffer<T>(iSlot)
    {
    }

    template<typename T>
    inline GeometryCBuffer<T>::GeometryCBuffer(T* pData, int iSlot) :
        ConstantBuffer<T>(pData, iSlot)
    {
    }

    template<typename T>
    inline GeometryCBuffer<T>::GeometryCBuffer(const CPtr<ID3D11Buffer>& pBuffer, int iSlot)    :
        ConstantBuffer<T>(pBuffer, iSlot)
    {
    }

    template<typename T>
    inline void GeometryCBuffer<T>::Bind()
    {
        ID3D11Buffer* pBuffer = ConstantBuffer<T>::GetBuffer().Get();
        Graphics::GetInst()->GetDeviceContext()->GSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, &pBuffer);
    }

    template<typename T>
    inline std::shared_ptr<Bindable> GeometryCBuffer<T>::Clone()
    {
        return std::shared_ptr<Bindable>();
    }

}
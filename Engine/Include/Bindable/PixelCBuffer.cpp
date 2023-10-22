#include "PixelCBuffer.h"

namespace Engine
{

    template<typename T>
    inline PixelCBuffer<T>::PixelCBuffer(int iSlot) :
        ConstantBuffer<T>(iSlot)
    {
    }

    template<typename T>
    inline PixelCBuffer<T>::PixelCBuffer(const T* pData, int iSlot) :
        ConstantBuffer<T>(pData, iSlot)
    {
    }

    template<typename T>
    inline void PixelCBuffer<T>::Update(float fDeltaTime)
    {
    }

    template<typename T>
    inline void PixelCBuffer<T>::Bind()
    {
        Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAdressof());
    }

    template<typename T>
    inline void PixelCBuffer<T>::GetAndBind()
    {
        std::shared_ptr<ID3D11Buffer>& pPrevBuffer = ConstantBuffer<T>::GetPrevBuffer();

        Graphics::GetInst()->GetDeviceContext()->PSGetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, &pPrevBuffer);

        Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAdressof());
    }

    template<typename T>
    inline void PixelCBuffer<T>::BindEnd()
    {
        Graphics::GetInst()->GetDeviceContext()->PSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::GetPrevBuffer().GetAdressof());
    }

    template<typename T>
    inline std::shared_ptr<Bindable> PixelCBuffer<T>::Clone()
    {
        return std::make_shared<PixelCBuffer<T>>(*this);
    }
}
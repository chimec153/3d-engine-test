#pragma once
#include "ConstantBuffer.h"

namespace Engine
{
    template <typename T>
    class ENGINE_DLL ComputeCBuffer :
        public ConstantBuffer<T>
    {
    public:
        ComputeCBuffer(int iSlot = 0);

    public:
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

    template<typename T>
    inline ComputeCBuffer<T>::ComputeCBuffer(int iSlot) :
        ConstantBuffer<T>(iSlot)
    {
    }

    template<typename T>
    inline void ComputeCBuffer<T>::Bind()
    {
        CPtr<ID3D11Buffer> pBuffer = ConstantBuffer<T>::GetBuffer();

        Graphics::GetInst()->GetDeviceContext()->CSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, pBuffer.GetAdressof());
    }

    template<typename T>
    inline std::shared_ptr<Bindable> ComputeCBuffer<T>::Clone()
    {
        return std::shared_ptr<Bindable>();
    }


}
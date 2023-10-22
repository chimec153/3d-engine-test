#include "VertexCBuffer.h"

namespace Engine
{
    template<typename T>
    inline VertexCBuffer<T>::VertexCBuffer(int iSlot) :
        ConstantBuffer<T>(iSlot)
    {
    }

    template<typename T>
    inline VertexCBuffer<T>::VertexCBuffer(const VertexCBuffer<T>& buffer) :
        ConstantBuffer<T>(buffer)
    {
    }

    template<typename T>
    inline VertexCBuffer<T>::~VertexCBuffer() noexcept
    {
    }

    template<typename T>
    inline void VertexCBuffer<T>::Update(float fDeltaTime)
    {
    }

    template<typename T>
    inline void VertexCBuffer<T>::Bind()
    {
        Graphics::GetInst()->GetDeviceContext()->VSSetConstantBuffers(ConstantBuffer<T>::GetSlot(), 1, ConstantBuffer<T>::pConstantBuffer.GetAdressof());
    }

    template<typename T>
    inline std::shared_ptr<Bindable> VertexCBuffer<T>::Clone()
    {
        return std::make_shared<VertexCBuffer<T>>(*this);
    }
}
#pragma once
#include "ConstantBuffer.h"

namespace Engine
{
    template <typename T>
    class ENGINE_DLL PixelCBuffer :
        public ConstantBuffer<T>
    {
    public:
        PixelCBuffer(int iSlot = 0);
        PixelCBuffer(const T* pData, int iSlot = 0);
        virtual ~PixelCBuffer() noexcept override = default;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        void GetAndBind();
        void BindEnd();
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
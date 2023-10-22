#pragma once
#include "ConstantBuffer.h"

namespace Engine
{
    template <typename T>
    class BindableManager;

    template <typename T>
    class ENGINE_DLL VertexCBuffer :
        public ConstantBuffer<T>
    {
        friend class BindableManager<VertexCBuffer<T>>;
    public:
        VertexCBuffer(int iSlot = 0);
        VertexCBuffer(const VertexCBuffer<T>& buffer);
        virtual ~VertexCBuffer() noexcept override;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
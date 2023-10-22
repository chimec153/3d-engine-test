#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL IndexBuffer :
        public Bindable
    {
        friend class BindableManager<IndexBuffer>;
        friend class Drawable;

    public:
        IndexBuffer(const std::vector<unsigned int>& pData);
        virtual ~IndexBuffer() noexcept override;

    private:
        CPtr<ID3D11Buffer> pIndexBuffer;
        std::vector<unsigned int> m_vecData;

    public:
        int GetSize()   const;
        const std::vector<unsigned int>& GetData()   const;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
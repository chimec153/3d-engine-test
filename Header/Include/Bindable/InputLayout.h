#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL InputLayout :
        public Bindable
    {
        friend class BindableManager<InputLayout>;
    public:
        InputLayout(const class std::shared_ptr<class VertexShader>& pShader, D3D11_INPUT_ELEMENT_DESC* pInputElement, int iCount, int iInstSize = 0);
        virtual ~InputLayout() noexcept override;

    private:
        CPtr<ID3D11InputLayout> pInputLayout;
        int m_iInstSize;

    public:
        int GetInstSize()   const;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

        // Sort-by-state cache — see VertexShader::ResetBoundCache.
        static void ResetBoundCache();

    private:
        static ID3D11InputLayout* s_pBound;
    };

}
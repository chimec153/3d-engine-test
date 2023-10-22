#pragma once
#include "Shader.h"
namespace Engine
{
    class ENGINE_DLL DomainShader :
        public Shader
    {
    public:
        DomainShader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~DomainShader() override = default;

    private:
        CPtr<ID3D11DomainShader>    m_pShader;
        CPtr<ID3D11DomainShader>    m_pPrevShader;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        void GetAndBind();
        void BindEnd();

    public:
        virtual void LoadShader() override;
    };

}
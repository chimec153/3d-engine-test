#pragma once
#include "Shader.h"
namespace Engine
{
    class ENGINE_DLL HullShader :
        public Shader
    {
    public:
        HullShader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~HullShader() override = default;

    private:
        CPtr<ID3D11HullShader>  m_pShader;
        CPtr<ID3D11HullShader>  m_pPrevShader;

    public:
        virtual void LoadShader() override;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        void GetAndBind();
        void BindEnd();


    };

}
#pragma once
#include "Shader.h"
namespace Engine
{
    class ENGINE_DLL GeometryShader :
        public Shader
    {
    public:
        GeometryShader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~GeometryShader() override = default;

    private:
        CPtr<ID3D11GeometryShader>  m_pGS;

    public:
        virtual void LoadShader() override;
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
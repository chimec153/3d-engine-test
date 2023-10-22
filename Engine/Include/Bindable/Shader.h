#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL Shader :
        public Bindable
    {
    public:
        Shader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~Shader()   override = default;

    private:
        CPtr<ID3DBlob> pBlob;
        std::unique_ptr<TCHAR[]> m_pFullPath;
        std::unique_ptr<char[]> m_pEntry;

    public:
        ID3DBlob* GetBlob()  const;
        bool LoadShaderFile(const char* pTarget);
        const std::unique_ptr<char[]>& GetEntry()  const;

    public:
        virtual void LoadShader() = 0;
    };

}
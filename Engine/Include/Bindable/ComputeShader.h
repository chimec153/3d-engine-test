#pragma once

#include "Shader.h"

namespace Engine
{
    class ENGINE_DLL ComputeShader :
        public Shader
    {
    public:
        ComputeShader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~ComputeShader() override = default;

    private:
        CPtr<ID3D11ComputeShader>   m_pComputeShader;

    public:
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
        virtual void LoadShader() override;
        virtual const char* GetTarget() const override { return "cs_5_0"; }
        virtual void CreateFromBlob() override;
    public:
        void Dispatch(int x = 1, int y = 1, int z = 1);
    };
}
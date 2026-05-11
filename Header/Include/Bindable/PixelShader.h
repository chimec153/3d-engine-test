#pragma once
#include "Shader.h"
namespace Engine
{
    class ENGINE_DLL PixelShader :
        public Shader
    {
    public:
        PixelShader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~PixelShader() noexcept override;

    private:
        CPtr<ID3D11PixelShader> pPixelShader;
        CPtr<ID3D11PixelShader> pPrevPixelShader;

    public:
        virtual void LoadShader() override;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual void PostBind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        void GetAndBind();
        void BindEnd();
        // Phase E7 — sort-by-state cache moved to Graphics::BindCache.
    };

}
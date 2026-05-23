#pragma once
#include "Shader.h"
namespace Engine
{
    class ENGINE_DLL VertexShader :
        public Shader
    {
    public:
        VertexShader(const TCHAR* pFilePath, const char* pEntry, std::shared_ptr<class InputLayout> pInputLayout = nullptr, std::shared_ptr<class InputLayout> pInputLayoutInst = nullptr);
        virtual ~VertexShader() noexcept override;

    private:
        CPtr<ID3D11VertexShader> pVertexShader;
        CPtr<ID3D11VertexShader> pPrevVertexShader;
        std::shared_ptr<class InputLayout> m_pInputLayout;
        std::shared_ptr<class InputLayout> m_pInputLayoutInst;

    public:
        std::shared_ptr<class InputLayout> GetInputLayout()   const;
        std::shared_ptr<class InputLayout> GetInstInputLayout()   const;
        // Back-fill the (instanced) input layout after the VS has been
        // constructed. BindableManager creates VS bindables before the
        // InputLayout pool exists, so the ctor's default-null layouts
        // get attached here as soon as the matching IL is registered.
        void SetInputLayout    (const std::shared_ptr<class InputLayout>& pIL) { m_pInputLayout     = pIL; }
        void SetInstInputLayout(const std::shared_ptr<class InputLayout>& pIL) { m_pInputLayoutInst = pIL; }

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
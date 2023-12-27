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
    };

}
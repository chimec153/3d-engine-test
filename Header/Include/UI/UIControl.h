#pragma once

#include "../Bindable/Drawable.h"

namespace Engine
{
    class ENGINE_DLL UIControl :
        public Drawable
    {
    public:
        UIControl(const std::string& strTexture);
        UIControl(const UIControl& control);
        virtual ~UIControl() override = default;

    private:
        UICBUFFER m_tCBuffer;
        std::shared_ptr<ConstantBuffer<UICBUFFER>> m_pCBuffer;

    public:
        void SetStartUV(const Vector2& vUV);
        void SetEndUV(const Vector2& vUV);
        void SetStartPos(const Vector2& vPos);
        void SetSize(const Vector2& vSize);
        void DrawQuad();

    public:
        virtual bool Init() override;
        virtual void Update(float fDelatTime) override;
        virtual void Bind() override;
    };
}
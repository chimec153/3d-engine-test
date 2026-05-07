#pragma once

#include "../Component/Component.h"
#include "../Types.h"

namespace Engine
{
    template <typename T> class ConstantBuffer;

    // Phase E5 — UIControl migrated from Drawable to Component shell.
    // Currently dead at runtime (the GameScene creation paths for the
    // UI hierarchy are commented out). Subclasses (Frame, Gauge, Image)
    // follow the same Component-shell pattern.
    class ENGINE_DLL UIControl :
        public Component
    {
    public:
        UIControl();
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
        virtual std::shared_ptr<Component> Clone() override;
    };
}

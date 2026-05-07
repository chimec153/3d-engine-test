#pragma once

#include "UIControl.h"

namespace Engine
{
    // Phase E5 — Image is a UIControl-derived Component shell.
    class ENGINE_DLL Image :
        public UIControl
    {
    public:
        Image();
        Image(const std::string& strTexture, const Vector2& vUVStart = { 0.f, 0.f }, const Vector2& vUVEnd = {1.f, 1.f});
        virtual ~Image() override = default;

    private:
        Vector2 m_vStart;
        Vector2 m_vEnd;

    public:
        virtual std::shared_ptr<Component> Clone() override;
    };
}

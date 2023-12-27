#pragma once

#include "UIControl.h"

namespace Engine
{
    class ENGINE_DLL Image :
        public UIControl
    {
    public:
        Image(const std::string& strTexture, const Vector2& vUVStart = { 0.f, 0.f }, const Vector2& vUVEnd = {1.f, 1.f});
        virtual ~Image() override = default;

    private:
        Vector2 m_vStart;
        Vector2 m_vEnd;



    };
}
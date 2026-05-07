#pragma once
#include "UIControl.h"
namespace Engine
{
    // Phase E5 — Gauge is a UIControl-derived Component shell.
    class ENGINE_DLL Gauge :
        public UIControl
    {
    public:
        virtual std::shared_ptr<Component> Clone() override
        {
            return std::make_shared<Gauge>(*this);
        }
    };
}

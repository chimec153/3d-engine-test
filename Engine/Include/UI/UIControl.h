#pragma once

#include "../Bindable/Drawable.h"

namespace Engine
{
    class ENGINE_DLL UIControl :
        public Drawable
    {
    public:
        UIControl();
        virtual ~UIControl() override = default;
    private:

    public:
    };
}
#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Decal :
        public Drawable
    {
    public:
        Decal();
        virtual ~Decal() override = default;
    };
}
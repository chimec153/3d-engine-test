#pragma once
#include "Bindable\Drawable.h"

namespace Client
{
    class Tree :
        public Engine::Drawable
    {
    public:
        Tree();
        virtual ~Tree() override = default;

    public:
        virtual bool Init() override;
    };

}
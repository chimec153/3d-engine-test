#pragma once
#include "Component\Component.h"

namespace Client
{
    // Phase E5 — Tree migrated from Drawable to Component shell. Currently
    // dead at runtime (the GameScene CreateDrawable<Tree> call is commented
    // out). Future re-introduction should pair this with a MeshRenderer +
    // a Transform-bearing GameObject and load the .obj into the renderer's
    // Mesh slot externally.
    class Tree :
        public Engine::Component
    {
    public:
        Tree();
        virtual ~Tree() override = default;

    public:
        virtual bool Init() override;
        virtual std::shared_ptr<Engine::Component> Clone() override;
    };

}

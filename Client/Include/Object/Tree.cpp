#include "Tree.h"

Client::Tree::Tree()
{
    SetComponentType(Engine::COMPONENT_TYPE::NONE);
}

bool Client::Tree::Init()
{
    if (!__super::Init())
        return false;

    // Phase E5 — Drawable::Load(TEXT("...Tree.obj")) used to populate the
    // Drawable's Bindable child list (Mesh + Material + Texture). For the
    // Component shell, this is a no-op; mesh/material assignment is the
    // responsibility of an outer GameObject + MeshRenderer setup.
    return true;
}

std::shared_ptr<Engine::Component> Client::Tree::Clone()
{
    return std::make_shared<Tree>(*this);
}

#include "Button.h"
#include "../Bindable/UIRenderer.h"
#include "../Bindable/Texture.h"

namespace Engine
{
    Button::Button()
        : UIControl()
    {
        Component::SetComponentType(COMPONENT_TYPE::NONE);
    }

    bool Button::Init()
    {
        // UIControl::Init creates the shared Transform child (UI mode,
        // pixel-space). Button only stands up the UIRenderer side; the
        // texture is supplied later via SetTexture from the caller.
        if (!UIControl::Init()) return false;
        m_pRenderer = AddUIRenderer("renderer", nullptr);
        return m_pRenderer != nullptr;
    }

    void Button::SetTexture(const std::shared_ptr<Texture>& pTex)
    {
        if (m_pRenderer) m_pRenderer->SetTexture(pTex);
    }

    void Button::OnMouseDown()
    {
        if (m_fnOnClick) m_fnOnClick();
    }

    std::shared_ptr<Component> Button::Clone()
    {
        return std::make_shared<Button>(*this);
    }
}

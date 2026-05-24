#include "Button.h"
#include "../Bindable/Transform.h"
#include "../Bindable/UIRenderer.h"
#include "../Bindable/Texture.h"
#include "../Input/Input.h"
#include "../Types.h"

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

    bool Button::HitTestMousePx() const
    {
        auto pTr = GetTransform();
        if (!pTr) return false;
        auto* pInput = CInput::GetInst();
        const float fMx = static_cast<float>(pInput->GetMouseX());
        const float fMy = static_cast<float>(pInput->GetMouseY());
        const Vector3 vPos   = pTr->GetPosition();
        const Vector3 vScale = pTr->GetScale();
        return fMx >= vPos.x && fMx <= vPos.x + vScale.x
            && fMy >= vPos.y && fMy <= vPos.y + vScale.y;
    }

    void Button::Update(float fDeltaTime)
    {
        // Ticks child Components (Transform / UIRenderer lifecycle).
        UIControl::Update(fDeltaTime);

        if (!m_fnOnClick) return;

        auto* pInput = CInput::GetInst();
        if (pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT) &&
            HitTestMousePx())
        {
            m_fnOnClick();
        }
    }

    std::shared_ptr<Component> Button::Clone()
    {
        return std::make_shared<Button>(*this);
    }
}

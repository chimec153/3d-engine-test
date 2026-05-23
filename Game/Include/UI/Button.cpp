#include "Button.h"
#include "Bindable/Transform.h"
#include "Bindable/UIRenderer.h"
#include "Bindable/Mesh.h"
#include "Bindable/Texture.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Input/Input.h"
#include "Core/Window.h"
#include "Types.h"

namespace Client
{
    Button::Button()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool Button::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        auto pVS       = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
        auto pPS       = Engine::StaticFindBindable<Engine::PixelShader> ("UIPS");
        auto pTopology = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
        auto pMesh     = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
        if (!pVS || !pPS || !pTopology || !pMesh) return false;

        m_pTransform = CreateComponent<Engine::Transform>("transform");
        if (m_pTransform)
        {
            // CAMERA_TYPE::UI so Transform::PostUpdate's NDC fallback
            // collapses matWorldViewProject to matWorld (Scale ×
            // Translation, no camera multiplication).
            m_pTransform->SetCameraType(Engine::CAMERA_TYPE::UI);
        }

        m_pRenderer = CreateComponent<Engine::UIRenderer>("renderer");
        if (m_pRenderer)
        {
            m_pRenderer->SetTarget(m_pTransform, pMesh, nullptr);
            m_pRenderer->SetVertexShader(pVS);
            m_pRenderer->SetPixelShader(pPS);
            m_pRenderer->SetTopology(pTopology);
            m_pRenderer->SetRenderLayer(Engine::RENDER_LAYER::UI);
        }

        return true;
    }

    void Button::SetRect(float fX, float fY, float fW, float fH)
    {
        m_fX = fX; m_fY = fY; m_fW = fW; m_fH = fH;
        if (m_pTransform)
        {
            m_pTransform->SetScale   (fW, fH, 1.f);
            m_pTransform->SetPosition(fX, fY, 0.f);
        }
    }

    void Button::SetTexture(const std::shared_ptr<Engine::Texture>& pTex)
    {
        if (m_pRenderer) m_pRenderer->SetTexture(pTex);
    }

    bool Button::HitTestMouseNDC() const
    {
        auto* pInput = Engine::CInput::GetInst();
        auto* pWin   = Engine::Window::GetInst();
        const float fW = static_cast<float>(pWin->GetWidth());
        const float fH = static_cast<float>(pWin->GetHeight());
        if (fW <= 0.f || fH <= 0.f) return false;

        // Window pixel coords → NDC (top-left pixel origin, y-down →
        // bottom-left NDC origin, y-up).
        const float fNdcX = (static_cast<float>(pInput->GetMouseX()) / fW) * 2.f - 1.f;
        const float fNdcY = 1.f - (static_cast<float>(pInput->GetMouseY()) / fH) * 2.f;

        return fNdcX >= m_fX && fNdcX <= m_fX + m_fW
            && fNdcY >= m_fY && fNdcY <= m_fY + m_fH;
    }

    void Button::Update(float fDeltaTime)
    {
        // Ticks child Components (Transform / UIRenderer lifecycle).
        Engine::UIControl::Update(fDeltaTime);

        if (!m_fnOnClick) return;

        auto* pInput = Engine::CInput::GetInst();
        if (pInput->IsMouseButtonDown(Engine::CInput::MOUSE_TYPE::LEFT) &&
            HitTestMouseNDC())
        {
            m_fnOnClick();
        }
    }

    std::shared_ptr<Engine::Component> Button::Clone()
    {
        return std::make_shared<Button>(*this);
    }
}

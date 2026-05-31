#include "Slider.h"
#include "Button.h"
#include "../Input/Input.h"
#include "../Bindable/Texture.h"
#include "../Bindable/BindableManager.h"
#include <algorithm>

namespace Engine
{
    // Named (not anonymous) so the unity/jumbo build — which concatenates this
    // file with other UI .cpp that have their own PackABGR/SolidTex — doesn't
    // merge the anonymous namespaces into one redefinition.
    namespace Slider_detail
    {
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Texture> SolidTex(const char* szTag, unsigned int uRGB, unsigned int uAlpha)
        {
            if (auto p = StaticFindBindable<Texture>(szTag)) return p;
            auto pNew = StaticCreateBindable<Texture>(szTag);
            if (!pNew) return nullptr;
            unsigned int uColor = PackABGR(uRGB, uAlpha);
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uColor;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        constexpr unsigned int kTrackRGB     = 0x2A2F33;
        constexpr unsigned int kHandleRGB    = 0x90CAF9;   // light blue
        constexpr unsigned int kTrackDimRGB  = 0x202225;
        constexpr unsigned int kHandleDimRGB = 0x5A6066;
    }

    Slider::Slider()
        : UIControl()
    {
        Component::SetComponentType(COMPONENT_TYPE::NONE);
    }

    bool Slider::Init()
    {
        using namespace Slider_detail;
        if (!UIControl::Init()) return false;

        // Child quads, no click handlers — all interaction is polled in Update.
        m_pTrack = CreateComponent<Button>("slider_track");
        if (m_pTrack) m_pTrack->SetTexture(SolidTex("slider_track", kTrackRGB, 0xC0));
        m_pHandle = CreateComponent<Button>("slider_handle");
        if (m_pHandle) m_pHandle->SetTexture(SolidTex("slider_handle", kHandleRGB, 0xFF));
        return true;
    }

    void Slider::SetSliderRect(float fX, float fY, float fW, float fH)
    {
        m_fRect[0] = fX; m_fRect[1] = fY; m_fRect[2] = fW; m_fRect[3] = fH;

        const float fTrackH = (std::max)(4.f, fH * 0.30f);
        const float fTrackY = fY + (fH - fTrackH) * 0.5f;
        m_fTrackRect[0] = fX; m_fTrackRect[1] = fTrackY;
        m_fTrackRect[2] = fW; m_fTrackRect[3] = fTrackH;
        if (m_pTrack) m_pTrack->SetRect(fX, fTrackY, fW, fTrackH);

        PlaceHandle();
    }

    void Slider::SetValue(float fValue)
    {
        const float lo = m_fMin, hi = m_fMax;
        m_fValue = fValue < lo ? lo : (fValue > hi ? hi : fValue);
        PlaceHandle();
    }

    void Slider::SetEnabled(bool bEnabled)
    {
        using namespace Slider_detail;
        m_bEnabled = bEnabled;
        if (!bEnabled) m_bDragging = false;
        if (m_pTrack)  m_pTrack->SetTexture(SolidTex(bEnabled ? "slider_track" : "slider_track_dim",
                                                     bEnabled ? kTrackRGB : kTrackDimRGB, 0xC0));
        if (m_pHandle) m_pHandle->SetTexture(SolidTex(bEnabled ? "slider_handle" : "slider_handle_dim",
                                                      bEnabled ? kHandleRGB : kHandleDimRGB, 0xFF));
    }

    void Slider::PlaceHandle()
    {
        const float x = m_fRect[0], y = m_fRect[1], w = m_fRect[2], h = m_fRect[3];
        if (w <= 0.f || h <= 0.f) return;

        const float lo = m_fMin, hi = m_fMax;
        float vc = m_fValue; if (vc < lo) vc = lo; if (vc > hi) vc = hi;
        const float t  = (hi > lo) ? (vc - lo) / (hi - lo) : 0.f;
        const float hw = (std::max)(10.f, h * 0.50f);
        const float hh = h * 0.80f;
        const float hx = x + t * (w - hw);
        const float hy = y + (h - hh) * 0.5f;
        m_fHandleRect[0] = hx; m_fHandleRect[1] = hy;
        m_fHandleRect[2] = hw; m_fHandleRect[3] = hh;
        if (m_pHandle) m_pHandle->SetRect(hx, hy, hw, hh);
    }

    void Slider::SetFromMouseX(float fMouseX)
    {
        const float lo = m_fMin, hi = m_fMax;
        const float hw = (std::max)(10.f, m_fRect[3] * 0.50f);
        const float usable = m_fRect[2] - hw;
        float t = (usable > 0.f) ? (fMouseX - m_fRect[0] - hw * 0.5f) / usable : 0.f;
        if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
        const float v = lo + t * (hi - lo);
        m_fValue = v;
        PlaceHandle();
        if (m_fnOnChange) m_fnOnChange(v);
    }

    void Slider::Update(float fDeltaTime)
    {
        UIControl::Update(fDeltaTime);
        if (!m_bEnabled) return;

        auto* pInput = CInput::GetInst();
        if (!pInput) return;

        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());

        const bool bOnHandle = mx >= m_fHandleRect[0] && mx <= m_fHandleRect[0] + m_fHandleRect[2]
                            && my >= m_fHandleRect[1] && my <= m_fHandleRect[1] + m_fHandleRect[3];
        // Track hit band is the full slider height so the thin groove is easy
        // to click.
        const bool bOnTrack = mx >= m_fRect[0] && mx <= m_fRect[0] + m_fRect[2]
                           && my >= m_fRect[1] && my <= m_fRect[1] + m_fRect[3];

        const bool bDownEdge = pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT);
        const bool bHeld     = pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT)
                            || pInput->IsMouseButtonPress(CInput::MOUSE_TYPE::LEFT);

        if (bDownEdge)
        {
            if (bOnHandle)      m_bDragging = true;
            else if (bOnTrack) { m_bDragging = true; SetFromMouseX(mx); }
        }
        if (!bHeld) m_bDragging = false;
        if (m_bDragging && bHeld) SetFromMouseX(mx);
    }

    std::shared_ptr<Component> Slider::Clone()
    {
        return std::make_shared<Slider>(*this);
    }
}

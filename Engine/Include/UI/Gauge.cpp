#include "Gauge.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/Transform.h"
#include "../Bindable/Texture.h"
#include "../Bindable/UIRenderer.h"
#include <algorithm>
#include <cstdio>

namespace Engine
{
    // Named namespace (not anonymous) so a jumbo/unity TU doesn't fold
    // EnsureSolidTexture into another file's anonymous scope. Same
    // reason the old HPBar/XPBar wrappers used HPBar_detail / XPBar_detail.
    namespace Gauge_detail
    {
        // 1x1 RGBA texture keyed by color value. Identical colors across
        // every Gauge instance share a single cached SRV.
        std::shared_ptr<Texture> EnsureSolidTexture(uint32_t uRGBA)
        {
            char szTag[32];
            std::snprintf(szTag, sizeof(szTag), "Gauge_%08X", uRGBA);

            if (auto p = StaticFindBindable<Texture>(szTag)) return p;
            auto pNew = StaticCreateBindable<Texture>(szTag);
            if (!pNew) return nullptr;

            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uRGBA;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }
    }

    Gauge::Gauge()
        : UIControl()
    {
        SetComponentType(COMPONENT_TYPE::NONE);
    }

    bool Gauge::Init()
    {
        if (!UIControl::Init()) return false;

        // Build the four children up front with placeholder rect/texture;
        // SetColors / SetRectPx (typically called right after AddComponent)
        // fill in the real values before the first draw.
        m_pTransformBG   = AddQuadTransform("transform_bg",   m_fX, m_fY, m_fW, m_fH);
        m_pTransformFill = AddQuadTransform("transform_fill", m_fX, m_fY, m_fW * m_fRatio, m_fH);
        m_pRendererBG    = AddUIRenderer  ("renderer_bg",   nullptr, m_pTransformBG);
        m_pRendererFill  = AddUIRenderer  ("renderer_fill", nullptr, m_pTransformFill);

        if (!m_pTransformBG || !m_pTransformFill || !m_pRendererBG || !m_pRendererFill)
            return false;

        ApplyColors();
        return true;
    }

    void Gauge::Update(float fDeltaTime)
    {
        UIControl::Update(fDeltaTime);

        if (m_pTransformFill)
            m_pTransformFill->SetScale(m_fW * m_fRatio, m_fH, 1.f);
    }

    void Gauge::SetColors(uint32_t uBG, uint32_t uFill)
    {
        m_uBG   = uBG;
        m_uFill = uFill;
        ApplyColors();
    }

    void Gauge::SetRectPx(float fX, float fY, float fW, float fH)
    {
        m_fX = fX;  m_fY = fY;  m_fW = fW;  m_fH = fH;
        ApplyRect();
    }

    void Gauge::SetRatio(float fRatio)
    {
        m_fRatio = std::clamp(fRatio, 0.f, 1.f);
    }

    void Gauge::ApplyColors()
    {
        if (m_pRendererBG)
        {
            if (auto p = Gauge_detail::EnsureSolidTexture(m_uBG))
                m_pRendererBG->SetTexture(p);
        }
        if (m_pRendererFill)
        {
            if (auto p = Gauge_detail::EnsureSolidTexture(m_uFill))
                m_pRendererFill->SetTexture(p);
        }
    }

    void Gauge::ApplyRect()
    {
        if (m_pTransformBG)
        {
            m_pTransformBG->SetPosition(m_fX, m_fY, 0.f);
            m_pTransformBG->SetScale   (m_fW, m_fH, 1.f);
        }
        if (m_pTransformFill)
        {
            m_pTransformFill->SetPosition(m_fX, m_fY, 0.f);
            m_pTransformFill->SetScale   (m_fW * m_fRatio, m_fH, 1.f);
        }
    }

    std::shared_ptr<Component> Gauge::Clone()
    {
        return std::make_shared<Gauge>(*this);
    }
}

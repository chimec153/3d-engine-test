#include "ScrollView.h"
#include "Button.h"
#include "../Input/Input.h"
#include "../Bindable/Texture.h"
#include "../Bindable/BindableManager.h"
#include <algorithm>

namespace Engine
{
    namespace
    {
        // ABGR memory layout (bytes R,G,B,A) for a 1x1 solid UI texture.
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        // Shared 1x1 solid texture, keyed by tag (so all ScrollViews reuse
        // the same track / thumb textures).
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
    }

    void ScrollView::SetViewport(float fX, float fY, float fW, float fH)
    {
        m_fViewport[0] = fX;
        m_fViewport[1] = fY;
        m_fViewport[2] = fW;
        m_fViewport[3] = fH;
        m_bDirty = true;
    }

    void ScrollView::AddItem(const std::shared_ptr<UIControl>& pItem,
                             float fX, float fY, float fW, float fH)
    {
        if (!pItem) return;
        m_items.push_back(Item{ pItem, fX, fY, fW, fH });
        m_bDirty = true;
    }

    void ScrollView::ClearItems()
    {
        m_items.clear();
        m_fScroll = 0.f;
        m_fScrollMax = 0.f;
        m_bDirty = true;
    }

    void ScrollView::RebuildContent()
    {
        const bool  bH      = (m_eAxis == Axis::Horizontal);
        const float vpStart = bH ? m_fViewport[0] : m_fViewport[1];
        const float vpEnd   = bH ? (m_fViewport[0] + m_fViewport[2])
                                 : (m_fViewport[1] + m_fViewport[3]);

        float fContentEnd = vpStart;   // rightmost / bottommost item edge
        float fFirstSize  = 0.f;
        bool  bFirst      = true;
        for (const auto& it : m_items)
        {
            const float fStart = bH ? it.x : it.y;
            const float fSize  = bH ? it.w : it.h;
            if (fStart + fSize > fContentEnd) fContentEnd = fStart + fSize;
            if (bFirst) { fFirstSize = fSize; bFirst = false; }
        }

        m_fScrollMax = (std::max)(0.f, fContentEnd - vpEnd);
        if (m_fScroll < -m_fScrollMax) m_fScroll = -m_fScrollMax;
        if (m_fScroll > 0.f)           m_fScroll = 0.f;

        m_fStepEff = (m_fStep > 0.f) ? m_fStep
                   : (fFirstSize > 0.f ? fFirstSize : 30.f);

        // Clip each item to the viewport so a partially-scrolled item is
        // trimmed at the edge (GPU scissor) instead of popping out whole. The
        // clip is constant per ScrollView, so set it once here.
        for (const auto& it : m_items)
            if (auto p = it.wp.lock())
                p->SetClipRect(m_fViewport[0], m_fViewport[1], m_fViewport[2], m_fViewport[3]);

        m_bDirty = false;
    }

    void ScrollView::ScrollBy(float fDelta)
    {
        m_fScroll += fDelta;
        if (m_fScroll > 0.f)           m_fScroll = 0.f;
        if (m_fScroll < -m_fScrollMax) m_fScroll = -m_fScrollMax;
    }

    void ScrollView::EnsureBar()
    {
        if (m_pTrack || !m_bShowBar) return;
        // Created as child Buttons (no click handler) — they render through
        // the normal child PreDraw walk. The track is a dim groove, the thumb
        // a brighter handle.
        m_pTrack = CreateComponent<Button>("scroll_track");
        if (m_pTrack) m_pTrack->SetTexture(SolidTex("scrollview_track", 0x202020, 0xA0));
        m_pThumb = CreateComponent<Button>("scroll_thumb");
        if (m_pThumb) m_pThumb->SetTexture(SolidTex("scrollview_thumb", 0xC0C0C0, 0xE0));
    }

    void ScrollView::UpdateBar()
    {
        if (!m_bShowBar) return;

        // No overflow → hide the bar (only touch it if it was already made).
        if (m_fScrollMax <= 0.f)
        {
            if (m_pTrack) m_pTrack->Disable();
            if (m_pThumb) m_pThumb->Disable();
            m_fThumbRect[0] = m_fThumbRect[1] = m_fThumbRect[2] = m_fThumbRect[3] = 0.f;
            return;
        }

        EnsureBar();
        if (!m_pTrack || !m_pThumb) return;
        m_pTrack->Enable();
        m_pThumb->Enable();

        const float vx = m_fViewport[0], vy = m_fViewport[1];
        const float vw = m_fViewport[2], vh = m_fViewport[3];
        const float frac = (-m_fScroll) / m_fScrollMax;   // 0 (start) .. 1 (end)

        if (m_eAxis == Axis::Horizontal)
        {
            const float barH   = (std::max)(4.f, vh * 0.10f);
            const float trackY = vy + vh - barH;            // along the bottom edge
            m_pTrack->SetRect(vx, trackY, vw, barH);

            const float content = vw + m_fScrollMax;
            float thumbW = vw * (vw / content);
            if (thumbW < 12.f) thumbW = 12.f;
            const float thumbX = vx + frac * (vw - thumbW);
            m_pThumb->SetRect(thumbX, trackY, thumbW, barH);
            m_fThumbRect[0] = thumbX; m_fThumbRect[1] = trackY;
            m_fThumbRect[2] = thumbW; m_fThumbRect[3] = barH;
        }
        else
        {
            const float barW   = (std::max)(4.f, vw * 0.10f);
            const float trackX = vx + vw - barW;            // along the right edge
            m_pTrack->SetRect(trackX, vy, barW, vh);

            const float content = vh + m_fScrollMax;
            float thumbH = vh * (vh / content);
            if (thumbH < 12.f) thumbH = 12.f;
            const float thumbY = vy + frac * (vh - thumbH);
            m_pThumb->SetRect(trackX, thumbY, barW, thumbH);
            m_fThumbRect[0] = trackX; m_fThumbRect[1] = thumbY;
            m_fThumbRect[2] = barW;   m_fThumbRect[3] = thumbH;
        }
    }

    void ScrollView::ApplyLayout()
    {
        const bool  bH  = (m_eAxis == Axis::Horizontal);
        const float vx0 = m_fViewport[0];
        const float vy0 = m_fViewport[1];
        const float vx1 = vx0 + m_fViewport[2];
        const float vy1 = vy0 + m_fViewport[3];

        for (const auto& it : m_items)
        {
            auto p = it.wp.lock();
            if (!p) continue;

            const float x = bH ? (it.x + m_fScroll) : it.x;
            const float y = bH ? it.y : (it.y + m_fScroll);

            // Visible if it overlaps the viewport at all — a partially-visible
            // item is kept and the GPU scissor (set in RebuildContent) trims
            // the part that spills past the edge, so items slide off smoothly
            // instead of popping out the moment they touch the boundary.
            const bool bVis = (x < vx1) && (x + it.w > vx0)
                           && (y < vy1) && (y + it.h > vy0);
            if (bVis)
            {
                p->Enable();
                p->SetRect(x, y, it.w, it.h);
            }
            else
            {
                p->Disable();
            }
        }
    }

    void ScrollView::Update(float fDeltaTime)
    {
        // Base Update keeps the component lifecycle normal; its mouse dispatch
        // self-skips because we never SetRect this control's own transform.
        UIControl::Update(fDeltaTime);

        if (m_bDirty) RebuildContent();

        if (m_fScrollMax > 0.f)
        {
            if (auto* pInput = CInput::GetInst())
            {
                const bool  bH = (m_eAxis == Axis::Horizontal);
                const float mx = static_cast<float>(pInput->GetMouseX());
                const float my = static_cast<float>(pInput->GetMouseY());

                // Wheel over the viewport: up steps back toward the start
                // (offset -> 0), down reveals more content (-> -scrollMax).
                const bool bOverVp = mx >= m_fViewport[0] && mx <= m_fViewport[0] + m_fViewport[2]
                                  && my >= m_fViewport[1] && my <= m_fViewport[1] + m_fViewport[3];
                const int dz = pInput->GetMouseDeltaZ();
                if (bOverVp && dz != 0)
                    ScrollBy(dz > 0 ? m_fStepEff : -m_fStepEff);

                // Drag the thumb. Grab on a left-down over the (last drawn)
                // thumb rect, follow the mouse while held, release on button up.
                const bool bOverThumb = mx >= m_fThumbRect[0] && mx <= m_fThumbRect[0] + m_fThumbRect[2]
                                     && my >= m_fThumbRect[1] && my <= m_fThumbRect[1] + m_fThumbRect[3];
                if (pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT) && bOverThumb)
                    m_bDragging = true;
                if (!pInput->IsMouseButtonPress(CInput::MOUSE_TYPE::LEFT)
                 && !pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT))
                    m_bDragging = false;
                if (m_bDragging)
                {
                    const float d        = bH ? static_cast<float>(pInput->GetMouseDeltaX())
                                              : static_cast<float>(pInput->GetMouseDeltaY());
                    const float vpLen    = bH ? m_fViewport[2] : m_fViewport[3];
                    const float thumbLen = bH ? m_fThumbRect[2] : m_fThumbRect[3];
                    const float travel   = vpLen - thumbLen;   // thumb pixels of range
                    if (travel > 0.f && d != 0.f)
                        ScrollBy(-d * (m_fScrollMax / travel));   // thumb-px -> content-px
                }
            }
        }

        ApplyLayout();
        UpdateBar();
    }

    std::shared_ptr<Component> ScrollView::Clone()
    {
        return std::make_shared<ScrollView>(*this);
    }
}

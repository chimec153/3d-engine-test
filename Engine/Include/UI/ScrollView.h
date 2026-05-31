#pragma once

#include "UIControl.h"
#include <memory>
#include <vector>

namespace Engine
{
    class Button;

    // Reusable scrolling viewport for UI rows / lists. It draws nothing of its
    // own — you register existing UIControl items (buttons, text, icons, …)
    // that were created and positioned on any host, give it a viewport rect,
    // and ScrollView re-places those items by a mouse-wheel-driven offset,
    // hiding (Disable) any that fall outside the viewport.
    //
    // There is no GPU scissor in this engine, so clipping is done by
    // Enable/Disable: an item only partially inside the viewport is hidden
    // whole. Items keep their original parent (so they render exactly as they
    // did before) — ScrollView only moves and shows/hides them.
    //
    // Usage:
    //   auto sv = host->CreateComponent<Engine::ScrollView>("row_sv");
    //   sv->SetAxis(Engine::ScrollView::Axis::Horizontal);
    //   sv->SetViewport(x, y, w, h);          // visible band, screen pixels
    //   for (each item already SetRect to its content position)
    //       sv->AddItem(item);                // captures its current rect
    //   // RebuildContent() is optional — it runs lazily on the next Update.
    class ENGINE_DLL ScrollView : public UIControl
    {
    public:
        enum class Axis { Horizontal, Vertical };

        ScrollView() = default;
        ScrollView(const ScrollView& other) = default;
        virtual ~ScrollView() override = default;

        // Visible window in screen pixels. Items outside it are hidden.
        void SetViewport(float fX, float fY, float fW, float fH);
        void SetAxis(Axis eAxis) { m_eAxis = eAxis; m_bDirty = true; }
        // Show the visible scrollbar (track + thumb) when content overflows.
        // On by default; pass false for wheel-only scrolling with no bar.
        void SetShowBar(bool b) { m_bShowBar = b; }
        // Distance scrolled per wheel notch. <= 0 (the default) auto-derives
        // it from the first registered item's size along the scroll axis, so
        // one notch advances by roughly one item.
        void SetScrollStep(float f) { m_fStep = f; }

        // Register an item with its content-space (unscrolled) rect, in screen
        // pixels. Place the item there yourself (SetRect); ScrollView re-places
        // it by the scroll offset and shows/hides it. Held weakly — a freed
        // item is skipped. Taking the rect explicitly (rather than reading the
        // item's Transform) keeps it correct for widgets like Text that lay
        // out their own box.
        void AddItem(const std::shared_ptr<UIControl>& pItem,
                     float fX, float fY, float fW, float fH);
        void ClearItems();

        // Recompute content extent + clamp the offset from the registered
        // items. Runs lazily on the next Update when items / axis / viewport
        // change, so an explicit call is optional.
        void RebuildContent();

        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        struct Item
        {
            std::weak_ptr<UIControl> wp;
            float x, y, w, h;   // content-space (unscrolled) rect
        };
        std::vector<Item> m_items;

        float m_fViewport[4] = { 0.f, 0.f, 0.f, 0.f };  // x, y, w, h
        Axis  m_eAxis        = Axis::Horizontal;
        float m_fScroll      = 0.f;    // current offset along the axis (<= 0)
        float m_fScrollMax   = 0.f;    // max scroll distance (content overflow)
        float m_fStep        = 0.f;    // caller-set notch distance; <=0 = auto
        float m_fStepEff     = 30.f;   // effective notch distance
        bool  m_bDirty       = true;

        // Visible scrollbar: a track + a thumb (child Buttons, created lazily).
        // The thumb reflects the scroll position and can be dragged. Hidden
        // when content fits or SetShowBar(false).
        bool  m_bShowBar     = true;
        std::shared_ptr<Button> m_pTrack;
        std::shared_ptr<Button> m_pThumb;
        float m_fThumbRect[4] = { 0.f, 0.f, 0.f, 0.f };  // last thumb rect (drag hit-test)
        bool  m_bDragging     = false;

        void ScrollBy(float fDelta);   // adjust + clamp m_fScroll
        void ApplyLayout();            // re-place + clip the registered items
        void EnsureBar();              // lazily create track + thumb
        void UpdateBar();              // size/place/show the bar from scroll state
    };
}

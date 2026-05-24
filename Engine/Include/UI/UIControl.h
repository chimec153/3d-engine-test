#pragma once

#include "../Component/Component.h"
#include "../Types.h"
#include <memory>

namespace Engine
{
    template <typename T> class ConstantBuffer;
    class Transform;
    class UIRenderer;
    class Texture;

    // UIControl is the Component-shell base for every game UI widget
    // (HPBar, XPBar, Button, Frame, Gauge, Image, …). Subclasses use
    // the protected AddUIRenderer / AddQuadTransform helpers to wire
    // their draw layers off the standard UI shader + quad mesh; the
    // base's m_pTransform is the widget's root placement in pixels.
    class ENGINE_DLL UIControl :
        public Component
    {
    public:
        UIControl();
        UIControl(const std::string& strTexture);
        UIControl(const UIControl& control);
        virtual ~UIControl() override;

    protected:
        // Shared UV-sub-region cbuffer (b5). Subclasses (Text et al)
        // that need to push their own UV sub-region — e.g. (0,0)-(1,1)
        // full quad to avoid inheriting EnemyCountHUD's per-digit cell —
        // mutate m_tCBuffer then bind m_pCBuffer. UIControl::Init does
        // the lazy lookup so a default-constructed subclass still has
        // the handle.
        UICBUFFER m_tCBuffer;
        std::shared_ptr<ConstantBuffer<UICBUFFER>> m_pCBuffer;

    private:
        // Shared placement for every UIControl subclass (Button, Text,
        // HPBar …). Lives as a child Component so the standard
        // hierarchy machinery (parent ↔ child Transform linking,
        // PostUpdate) picks it up automatically. Coordinates are
        // *pixels* — Transform::PostUpdate converts to NDC when the
        // camera type is UI.
        std::shared_ptr<class Transform> m_pTransform;

        // Last anchor spec passed to SetRectByAnchor[Frac]. Stored so
        // RecomputeRectFromAnchor (called on Window resize) can re-derive
        // the pixel rect without callers re-pushing every frame. bSet
        // gates resize-driven recomputation so plain SetRect users
        // aren't disturbed.
        struct AnchorSpec
        {
            Vector2 vAnchor   { 0.f, 0.f }; // 0..1 of window
            Vector2 vPivot    { 0.f, 0.f }; // 0..1 of own rect
            Vector2 vSize     { 0.f, 0.f }; // pixels or window-fraction
            bool    bSizeFrac = false;
            bool    bSet      = false;
        } m_tAnchor;

        // Window::RegisterResizeCallback token. -1 = not registered;
        // dtor unregisters so a destroyed UIControl can't be re-entered
        // on a later WM_SIZE.
        int m_iResizeToken = -1;

    public:
        // Rect in screen pixels: (fX, fY) top-left, (fW, fH) size.
        // Transform's UI camera path turns this into the right NDC
        // clip matrix for the UI VS — callers think in pixels.
        void SetRect(float fX, float fY, float fW, float fH);

        // Anchor-relative placement. vAnchor is a point in [0,1]² of the
        // window (e.g. (0,0)=top-left, (1,1)=bottom-right). vPivot is the
        // matching point on the *own* rect (e.g. (0,1)=own bottom-left
        // meets the anchor — handy for HUD bars that grow upward from a
        // bottom margin). vSizePx is the rect size in pixels. The spec
        // is cached; window resize re-applies it via OnRectChanged.
        void SetRectByAnchor(Vector2 vAnchor, Vector2 vPivot, Vector2 vSizePx);

        // Same as above but vSizeFrac is in window-fractions
        // (vSizeFrac.x * windowW, vSizeFrac.y * windowH). Useful for
        // HUD widgets whose size should track resolution.
        void SetRectByAnchorFrac(Vector2 vAnchor, Vector2 vPivot, Vector2 vSizeFrac);

        virtual std::shared_ptr<class Transform> GetTransform() const override { return m_pTransform; }

        void SetStartUV(const Vector2& vUV);
        void SetEndUV(const Vector2& vUV);
        void SetStartPos(const Vector2& vPos);
        void SetSize(const Vector2& vSize);
        void DrawQuad();

    protected:
        // Wires a child UIRenderer Component to the standard UI shaders
        // ("UIVS"/"UIPS"), TriangleStrip topology and the "UIQuad" mesh.
        // Targets pTransform when provided, otherwise the UIControl's
        // own m_pTransform. pTex may be null (Button defers SetTexture).
        // Returns nullptr if any required Bindable is missing.
        std::shared_ptr<UIRenderer> AddUIRenderer(
            const std::string& strTag,
            const std::shared_ptr<Texture>& pTex,
            const std::shared_ptr<Transform>& pTransform = nullptr);

        // Creates a child Transform in UI camera mode with the given
        // pixel rect. Used by multi-quad subclasses (HPBar / XPBar)
        // that need additional Transforms beyond the base one.
        std::shared_ptr<Transform> AddQuadTransform(
            const std::string& strTag,
            float fX, float fY, float fW, float fH);

        // Resolved pixel rect notification. Default impl calls SetRect
        // on the base Transform. Subclasses with multiple child quads
        // (Gauge → BG/Fill) override to fan the rect out themselves.
        // Called from SetRect/SetRectByAnchor[Frac] and from the
        // Window resize callback.
        virtual void OnRectChanged(float fX, float fY, float fW, float fH);

        // Mouse-event hooks dispatched from UIControl::Update each frame
        // when the cursor falls inside the widget's pixel rect. Default
        // no-ops; widgets that care (Button) override.
        //   OnHover     — every frame the cursor is inside the rect.
        //   OnMouseDown — single edge: left button transitions to "down"
        //                 this frame *and* the cursor is inside.
        // Polled regardless of the game-pause/time-scale gate so a
        // Button can still be clicked inside a paused modal (level-up
        // choice screen).
        virtual void OnHover() {}
        virtual void OnMouseDown() {}

        // True when the OS mouse position (window pixels) is within the
        // current rect of the base Transform. Provided to subclasses
        // that want to gate custom logic on hover (Button's click test
        // delegates to this).
        bool HitTestMousePx() const;

    private:
        // Resolves m_tAnchor against the current Window size and
        // dispatches the resulting pixel rect through OnRectChanged.
        // No-op if no anchor spec has been set.
        void RecomputeRectFromAnchor();

    public:
        virtual bool Init() override;
        virtual void Update(float fDelatTime) override;
        virtual std::shared_ptr<Component> Clone() override;
    };
}

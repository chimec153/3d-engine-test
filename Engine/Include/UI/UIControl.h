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
        virtual ~UIControl() override = default;

    private:
        UICBUFFER m_tCBuffer;
        std::shared_ptr<ConstantBuffer<UICBUFFER>> m_pCBuffer;

        // Shared placement for every UIControl subclass (Button, Text,
        // HPBar …). Lives as a child Component so the standard
        // hierarchy machinery (parent ↔ child Transform linking,
        // PostUpdate) picks it up automatically. Coordinates are
        // *pixels* — Transform::PostUpdate converts to NDC when the
        // camera type is UI.
        std::shared_ptr<class Transform> m_pTransform;

    public:
        // Rect in screen pixels: (fX, fY) top-left, (fW, fH) size.
        // Transform's UI camera path turns this into the right NDC
        // clip matrix for the UI VS — callers think in pixels.
        void SetRect(float fX, float fY, float fW, float fH);
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

    public:
        virtual bool Init() override;
        virtual void Update(float fDelatTime) override;
        virtual std::shared_ptr<Component> Clone() override;
    };
}

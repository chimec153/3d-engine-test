#pragma once
#include "../Component/Component.h"
#include "../Types.h"

namespace Engine
{
    template <typename T> class ConstantBuffer;

    // Phase E5 — UIRenderer migrated from Drawable to Component. Currently
    // dead at runtime (consumed only via Inventory, whose construction
    // path in GameScene is commented out). Kept as a usable shell for
    // future GameObject-based UI rendering.
    class ENGINE_DLL UIRenderer :
        public Component
    {
    public:
        UIRenderer();
        UIRenderer(const UIRenderer& other);
        virtual ~UIRenderer() override = default;

    private:
        std::shared_ptr<class Camera>     m_pCamera;
        std::shared_ptr<class Transform>  m_pParentTransform;
        std::shared_ptr<class Mesh>       m_pParentMesh;
        std::shared_ptr<class Animation>  m_pParentAnimation;
        std::shared_ptr<class Light>      m_pLight;
        RENDER_LAYER m_eRenderLayer;

        // Phase E5 — Bind used to inherit shader/topology/texture state
        // from the Drawable hierarchy (UIRenderer-as-Drawable's child
        // list). With Drawable gone these slots are explicit on
        // UIRenderer so a Component-only UI element (HPBar, future
        // Image revival, etc.) can drive its draw entirely through here.
        std::shared_ptr<class VertexShader> m_pVS;
        std::shared_ptr<class PixelShader>  m_pPS;
        std::shared_ptr<class Texture>      m_pTexture;
        std::shared_ptr<class Topology>     m_pTopology;

        // First-class UI tint (like a UMG brush/widget ColorAndOpacity).
        // Bind() pushes it into the shared UITint cbuffer right before this
        // renderer's draw, so each UI element shows its own colour. Default
        // white = no tint. Only PS_UITint (text) samples it; other UI pixel
        // shaders ignore the bound cbuffer harmlessly.
        Vector4 m_vTint{ 1.f, 1.f, 1.f, 1.f };
        // Shared engine cbuffers, looked up lazily in Bind: UITint (tint)
        // and the b5 "UI" cbuffer (reset to a full-quad UV so a glyph-atlas
        // element samples its whole texture, undoing any sub-region a
        // sibling like EnemyCountHUD left behind).
        std::shared_ptr<ConstantBuffer<UITINTBUFFER>> m_pTintCBuffer;
        std::shared_ptr<ConstantBuffer<UICBUFFER>>    m_pUICBuffer;

        // Optional scissor clip (screen px) applied around this renderer's
        // draw in Bind(). m_bClip off = no clip (the default).
        bool  m_bClip    = false;
        float m_fClip[4] = { 0.f, 0.f, 0.f, 0.f };  // x, y, w, h
        // Shared scissor-enabled rasterizer state (cache tag "UIScissor"),
        // fetched lazily in Bind like the cbuffers above — points at the one
        // cached instance, not a per-renderer copy. Left out of the copy ctor
        // so a clone re-fetches it.
        std::shared_ptr<class RasterizerState> m_pScissorRS;

    public:
        void SetCamera(std::shared_ptr<class Camera> pCamera);
        // Phase E7 — SetTarget(Drawable) replaced by individual setters
        // since the Drawable type is gone. Sole live caller path
        // (Inventory) is currently commented out anyway; this keeps the
        // shape ready for a GameObject-based revival.
        void SetTarget(std::shared_ptr<class Transform> pTransform,
                       std::shared_ptr<class Mesh> pMesh,
                       std::shared_ptr<class Animation> pAnimation);

        // Optional per-renderer shader / texture / topology — Bind binds
        // them before the Mesh draw so callers don't need to do it.
        void SetVertexShader(const std::shared_ptr<class VertexShader>& p) { m_pVS = p; }
        void SetPixelShader (const std::shared_ptr<class PixelShader>&  p) { m_pPS = p; }
        void SetTexture     (const std::shared_ptr<class Texture>&      p) { m_pTexture = p; }
        void SetTopology    (const std::shared_ptr<class Topology>&     p) { m_pTopology = p; }

        void SetRenderLayer(RENDER_LAYER eLayer) { m_eRenderLayer = eLayer; }
        RENDER_LAYER GetRenderLayer() const { return m_eRenderLayer; }

        // Per-instance tint colour, 0xRRGGBBAA (UMG-style ColorAndOpacity).
        // Bound right before this renderer's draw. Default white.
        void SetTint(unsigned int uRGBA);

        // Optional GPU scissor clip in screen pixels. When set, Bind() limits
        // this renderer's draw to the rect (ScrollView uses it to trim items
        // to its viewport so a partially-scrolled item is clipped, not hidden).
        void SetClipRect(float fX, float fY, float fW, float fH);
        void ClearClipRect() { m_bClip = false; }

    public:
        virtual bool Init() override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Self-contained bind+draw called from RenderManager's UI pass.
        void Bind();
    };

}

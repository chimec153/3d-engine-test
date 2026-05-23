#pragma once
#include "../Component/Component.h"
#include "../Types.h"

namespace Engine
{
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

    public:
        virtual bool Init() override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

        // Bind is NOT a virtual override — Component has no Bind interface.
        // Self-contained bind+draw called from RenderManager's UI pass.
        void Bind();
    };

}

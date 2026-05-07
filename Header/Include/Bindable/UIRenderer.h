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

    public:
        void SetCamera(std::shared_ptr<class Camera> pCamera);
        void SetTarget(std::shared_ptr<class Drawable> pTarget);

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

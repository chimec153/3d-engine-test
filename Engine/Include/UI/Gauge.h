#pragma once
#include "UIControl.h"

namespace Engine
{
    class Transform;
    class UIRenderer;
    class Texture;

    // Two-quad UI gauge (BG track + Fill bar). Generalizes the pattern
    // HPBar/XPBar used to duplicate: a grey background quad with a
    // colored fill quad whose horizontal scale tracks a 0..1 ratio
    // pushed in from the outside.
    //
    // Lifecycle:
    //   AddComponent<Gauge> → Init creates 4 child Components
    //     (BG/Fill Transforms + UIRenderers, no texture / zero rect yet).
    //   SetColors / SetRectPx — call after AddComponent; both apply
    //     immediately to the already-created children. Texture cache
    //     key is derived from the color so identical colors share SRVs.
    //   SetRatio(0..1) each frame from the owner (Scene) — Update
    //     rescales the Fill Transform to fullW * ratio.
    class ENGINE_DLL Gauge :
        public UIControl
    {
    public:
        Gauge();
        virtual ~Gauge() override = default;

        // 0xAABBGGRR (D3D11 R8G8B8A8 memory layout). Safe to call before
        // or after Init — re-binds the texture immediately if children exist.
        void SetColors(uint32_t uBG, uint32_t uFill);

        // Pixel-space rect; (fX, fY) top-left, (fW, fH) size. The Fill
        // quad's width is later multiplied by the current ratio.
        void SetRectPx(float fX, float fY, float fW, float fH);

        // Clamped to [0, 1].
        void SetRatio(float fRatio);

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        std::shared_ptr<Transform>  m_pTransformBG;
        std::shared_ptr<Transform>  m_pTransformFill;
        std::shared_ptr<UIRenderer> m_pRendererBG;
        std::shared_ptr<UIRenderer> m_pRendererFill;

        uint32_t m_uBG    = 0xFF303030;
        uint32_t m_uFill  = 0xFFFFFFFF;
        float    m_fX     = 0.f;
        float    m_fY     = 0.f;
        float    m_fW     = 0.f;
        float    m_fH     = 0.f;
        float    m_fRatio = 1.f;

        void ApplyColors();
        void ApplyRect();
    };
}

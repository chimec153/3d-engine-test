#pragma once
#include "Vector3.h"
#include "../WeaponData.h"   // TrailStyle
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>
#include <deque>

namespace Engine
{
    class Transform;
    class VertexShader;
    class PixelShader;
    class InputLayout;
    class Topology;
    class RasterizerState;
    class BlendState;
    class DepthStencilState;
}

namespace Client
{
    // Bullet tracer trails. Where Beam is a static fixed-endpoint line, a trail
    // is a dynamic ribbon that follows a moving bullet: head thick + bright,
    // tapering thinner + dimmer toward the tail. Each bullet keeps a short
    // position history (distance-gated deque) and Submit()s it every frame; the
    // history's curve is what makes spiral / orbital / homing bullets leave a
    // bent streak instead of a straight line.
    //
    // Reuses the laser-beam pipeline (BeamVS / BeamPS / BeamVtx / AccBlend), so
    // the cross-section falloff, noise scroll and soft-particle depth fade all
    // come for free. The along-ribbon UV is accumulated path distance, so the
    // texture / noise flows evenly regardless of segment length. A small
    // additive glow sprite (BeamGlowPS) is drawn at each head for the classic
    // "tracer tip". Additive (ONE/ONE) — the tail fades by tapering per-vertex
    // colour to 0 (alpha is ignored by additive blend).
    //
    // Singleton like BeamRenderManager so Bullet (submit) and the renderer
    // (draw) share one batch without threading a pointer.
    // Resolved look for one TrailStyle. iMaxPoints <= 0 means "no trail" (the
    // bullet skips its history + submit entirely). Bullet reads iMaxPoints /
    // fMinDist to size its history; Render reads the rest to build the ribbon.
    struct TrailPreset
    {
        int   iMaxPoints;      // history length cap (ribbon segment count + 1)
        float fMinDist;        // commit a new point only after moving this far
        float fHeadWidthMul;   // head half-width = bullet radius * this
        float fTailWidthFrac;  // tail half-width = head * this
        float fHeadIntensity;  // HDR head brightness (feeds bloom)
        float fGlowScale;      // head glow radius vs head half-width
        float fGlowIntensity;  // HDR head-glow brightness
    };

    class TrailRenderManager
    {
        static TrailRenderManager* m_pInst;

    public:
        // Preset table lookup (single source of truth, shared by Bullet's
        // history sizing and the renderer). Returns a static const ref.
        static const TrailPreset& GetPreset(TrailStyle eStyle);

        static TrailRenderManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new TrailRenderManager;
            return m_pInst;
        }

        static void DestroyInst()
        {
            if (m_pInst)
            {
                delete m_pInst;
                m_pInst = nullptr;
            }
        }

        // Resolve the shared render resources (idempotent).
        bool Init();

        // Queue one bullet's trail for this frame. path.front() is the head
        // (newest position), path.back() the tail. fBulletRadius is the
        // bullet's half-width; the eStyle preset scales width / brightness /
        // length / glow off it.
        void Submit(const std::deque<Engine::Vector3>& path,
                    const Engine::Vector3& vColor, float fBulletRadius,
                    TrailStyle eStyle);

        // Draw every queued trail (ribbons + head glows), then clear the queue.
        // Call from a RenderManager ALPHA custom-render callback.
        void Render();

    private:
        TrailRenderManager() = default;
        ~TrailRenderManager() = default;
        TrailRenderManager(const TrailRenderManager&) = delete;
        TrailRenderManager& operator=(const TrailRenderManager&) = delete;

        struct Trail
        {
            std::vector<Engine::Vector3> path;   // [0] = head .. [n-1] = tail
            Engine::Vector3 vColor;
            float           fBulletRadius = 0.f;
            TrailStyle      eStyle = TrailStyle::Tracer;
        };

        // GPU vertex — matches the "BeamVtx" input layout (pos@0 / uv@12 /
        // color@20, 36-byte stride), shared with the beam.
        struct TrailVertex
        {
            float px, py, pz;
            float u, v;
            float r, g, b, a;
        };

        std::vector<Trail>        m_subs;
        std::vector<TrailVertex>  m_ribbon;   // reused scratch
        std::vector<TrailVertex>  m_glows;    // reused scratch
        bool m_bInit = false;

        std::shared_ptr<Engine::Transform>         m_pVP;
        std::shared_ptr<Engine::VertexShader>      m_pVS;
        std::shared_ptr<Engine::PixelShader>       m_pPS;       // ribbon (BeamPS)
        std::shared_ptr<Engine::PixelShader>       m_pGlowPS;   // head sprite (BeamGlowPS)
        std::shared_ptr<Engine::InputLayout>       m_pIL;
        std::shared_ptr<Engine::Topology>          m_pTopo;
        std::shared_ptr<Engine::RasterizerState>   m_pNoCull;
        std::shared_ptr<Engine::BlendState>        m_pAddBlend;
        std::shared_ptr<Engine::DepthStencilState> m_pNoDepthWrite;
    };
}

#pragma once
#include "Vector3.h"
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>

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
    // Laser beam renderer. Replaces the old per-beam AxisBox MeshRenderer with
    // camera-facing additive billboards so the beam reads as glowing energy
    // rather than a lit solid box.
    //
    // Each frame every active Beam Submit()s its segment; Render() (an ALPHA
    // custom-render callback) builds the geometry CPU-side and draws all beams
    // in a few batched additive passes:
    //   * Geometry  — one camera-facing quad per beam, its width axis =
    //                 cross(beamDir, eyeToBeam) so it rotates around the beam
    //                 to face the camera.
    //   * Layering  — each beam is drawn as a wide saturated outer glow, a mid
    //                 body, and a thin near-white hot core (BeamVS/BeamPS
    //                 reused; only width + colour change per layer).
    //   * HDR bloom — core colours exceed 1.0, so the existing bloom pass
    //                 haloes them. Additive (ONE/ONE) blend, no depth write.
    //   * Soft particles — BeamPS fades against scene depth (t10, enabled in
    //                 RenderManager::RenderAlpha) at wall / floor intersections.
    //
    // Singleton like FootstepManager / HealAuraManager so Beam (submit) and the
    // renderer (draw) share one batch without threading a pointer.
    class BeamRenderManager
    {
        static BeamRenderManager* m_pInst;

    public:
        static BeamRenderManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new BeamRenderManager;
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

        // Resolve the shared render resources (idempotent). Safe to call on
        // every scene (re)load.
        bool Init();

        // Queue one beam segment for this frame. vColor is the beam's base
        // colour in 0..1 (intensity / whitening are applied per layer).
        void Submit(const Engine::Vector3& vStart, const Engine::Vector3& vEnd,
                    float fHalfWidth, const Engine::Vector3& vColor);

        // Draw every queued beam, then clear the queue. Call from a
        // RenderManager ALPHA custom-render callback.
        void Render();

    private:
        BeamRenderManager() = default;
        ~BeamRenderManager() = default;
        BeamRenderManager(const BeamRenderManager&) = delete;
        BeamRenderManager& operator=(const BeamRenderManager&) = delete;

        struct Seg
        {
            Engine::Vector3 vStart;
            Engine::Vector3 vEnd;
            float           fHalfWidth = 0.f;
            Engine::Vector3 vColor;     // base colour, 0..1
        };

        // GPU vertex — must match the "BeamVtx" input layout (pos@0 / uv@12 /
        // color@20, 36-byte stride) registered in BindableManager.
        struct BeamVertex
        {
            float px, py, pz;   // world position
            float u, v;         // u along length, v across width
            float r, g, b, a;   // colour * intensity (HDR)
        };

        std::vector<Seg>        m_subs;
        std::vector<BeamVertex> m_verts;   // reused scratch buffer
        bool m_bInit = false;

        std::shared_ptr<Engine::Transform>        m_pVP;    // identity NORMAL -> g_matTransform = VP
        std::shared_ptr<Engine::VertexShader>     m_pVS;
        std::shared_ptr<Engine::PixelShader>      m_pPS;
        std::shared_ptr<Engine::InputLayout>      m_pIL;
        std::shared_ptr<Engine::Topology>         m_pTopo;
        std::shared_ptr<Engine::RasterizerState>  m_pNoCull;
        std::shared_ptr<Engine::BlendState>       m_pAddBlend;
        std::shared_ptr<Engine::DepthStencilState> m_pNoDepthWrite;
    };
}

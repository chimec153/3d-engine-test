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
    // Procedural muzzle flash: a single camera-facing additive billboard popped
    // at the gun barrel when a weapon fires. The 8-spoke starburst is drawn
    // entirely in the pixel shader from polar coordinates (no texture) -- see
    // PS_Muzzle in Beam.fx. Reuses the Beam.fx pipeline (BeamVS / BeamVtx) like
    // DeathBurstManager: one quad per flash, additive (ONE/ONE) so the hot core
    // (HDR colour > 1) feeds the bloom post-process.
    //
    // Per flash the CPU bakes: a random seed (rotates the spokes so repeated
    // shots differ) into the vertex color.a, and a fast-decay life envelope into
    // the HDR tint (color.rgb) so the flash reads as a one-frame pop that
    // vanishes over ~0.06s.
    //
    // Pooled + aged + singleton like the other VFX managers (Footstep / Beam /
    // DeathBurst). Replaces the old particle-burst muzzle in VfxManager.
    class MuzzleFlashManager
    {
        static MuzzleFlashManager* m_pInst;

    public:
        static MuzzleFlashManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new MuzzleFlashManager;
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

        // Resolve the shared render resources (idempotent). Always resets the
        // pool. Safe to call on every scene (re)load.
        bool Init();

        // Pop a muzzle flash at a world position (the gun barrel). vDir is the
        // horizontal fire direction (world space); the flash is oriented +
        // elongated along it so it blasts one way instead of a symmetric star.
        void Spawn(const Engine::Vector3& vPos, const Engine::Vector3& vDir);

        // Age every active flash; deactivate the expired.
        void Update(float fDeltaTime);

        // Draw all active flashes, then they self-expire. Call from a
        // RenderManager ALPHA custom-render callback.
        void Render();

        // Deactivate everything (scene change).
        void Clear();

    private:
        MuzzleFlashManager() = default;
        ~MuzzleFlashManager() = default;
        MuzzleFlashManager(const MuzzleFlashManager&) = delete;
        MuzzleFlashManager& operator=(const MuzzleFlashManager&) = delete;

        // Cheap inline LCG -- <random> is banned here (epsilon macro). Same
        // generator DeathBurstManager / FragmentShatterManager use.
        float Rand();

        struct Flash
        {
            Engine::Vector3 vPos;
            Engine::Vector3 vDir;          // horizontal fire direction (unit)
            float fAge = 0.f, fLife = 0.f, fSize = 0.f, fSeed = 0.f;
            bool  bActive = false;
        };

        // Matches the "BeamVtx" input layout (pos@0 / uv@12 / color@20, 36B).
        // color.rgb = HDR tint * life envelope, color.a = per-flash seed (0..1).
        struct MuzzleVertex { float px, py, pz; float u, v; float r, g, b, a; };

        // Build one oriented quad: uv.x runs along vAxisF (forward), uv.y along
        // vAxisS (side), with separate half-extents so the flash elongates the
        // way it fires.
        void BuildQuad(std::vector<MuzzleVertex>& out,
                       const Engine::Vector3& vCentre, float fHalfF, float fHalfS,
                       const Engine::Vector3& vAxisF, const Engine::Vector3& vAxisS,
                       float tintR, float tintG, float tintB, float fSeed);

        std::vector<Flash>        m_flashes;
        std::vector<MuzzleVertex> m_verts;   // reused scratch buffer
        int m_iNext = 0;
        unsigned int m_uSeed = 1u;
        bool m_bInit = false;

        std::shared_ptr<Engine::Transform>         m_pVP;   // identity NORMAL -> g_matTransform = VP
        std::shared_ptr<Engine::VertexShader>      m_pVS;
        std::shared_ptr<Engine::PixelShader>       m_pPS;
        std::shared_ptr<Engine::InputLayout>       m_pIL;
        std::shared_ptr<Engine::Topology>          m_pTopo;
        std::shared_ptr<Engine::RasterizerState>   m_pNoCull;
        std::shared_ptr<Engine::BlendState>        m_pAddBlend;
        std::shared_ptr<Engine::DepthStencilState> m_pNoDepthWrite;
    };
}

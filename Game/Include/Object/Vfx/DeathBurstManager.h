#pragma once
#include "Vector3.h"
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>
#include <unordered_map>

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
    class Texture;
}

namespace Client
{
    // Stylized enemy-death burst: a flat-colour billboard puff cloud, star /
    // diamond sparkles, and a hard-edge expanding smoke ring. All are
    // camera-facing billboards drawn in the ALPHA pass (puff + ring
    // alpha-blended, sparkles additive for bloom), reusing the Beam.fx
    // pipeline (BeamVS / BeamVtx) with per-shape mask pixel shaders.
    //
    // Colour comes from a 1xN gradient ramp LUT sampled by each particle's
    // normalised lifetime (carried in the vertex's color.a), point-sampled so
    // the ramp's hard steps survive as flat toon colour banding. The puff ramp
    // is generated per enemy colour (cached) so each enemy type bursts in its
    // own palette with the same masks; sparkles use a shared white/gold ramp,
    // the ring a shared smoke ramp.
    //
    // Pooled + aged like FootstepManager (SpawnBurst seeds instances; Update
    // ages them; Render draws active ones), singleton like the other VFX
    // managers.
    class DeathBurstManager
    {
        static DeathBurstManager* m_pInst;

    public:
        static DeathBurstManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new DeathBurstManager;
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

        // Build shared render resources + the sparkle / smoke ramps. Idempotent
        // (resources built once); always resets the pools.
        bool Init();

        // Fire a death burst at vPos. fScale is the enemy's body scale (sizes
        // the puffs / sparkles / ring); vColor is the enemy's colour (picks the
        // per-type puff ramp).
        void SpawnBurst(const Engine::Vector3& vPos, float fScale,
                        const Engine::Vector3& vColor);

        // Age every active particle; deactivate the expired.
        void Update(float fDeltaTime);

        // Draw all active particles. Call from a RenderManager ALPHA
        // custom-render callback.
        void Render();

        // Deactivate everything (scene change).
        void Clear();

    private:
        DeathBurstManager() = default;
        ~DeathBurstManager() = default;
        DeathBurstManager(const DeathBurstManager&) = delete;
        DeathBurstManager& operator=(const DeathBurstManager&) = delete;

        // Cheap inline LCG — <random> is banned here (epsilon macro). Same
        // generator FragmentShatterManager uses.
        float Rand();
        float Rand(float a, float b) { return a + (b - a) * Rand(); }

        // 1xN stepped ramp builders. GetPuffRamp caches one per quantised
        // colour so same-type deaths reuse a texture.
        std::shared_ptr<Engine::Texture> GetPuffRamp(const Engine::Vector3& vColor);
        std::shared_ptr<Engine::Texture> MakeRamp(const std::string& strTag, int iKind,
                                                  const Engine::Vector3& vBase);

        struct Puff
        {
            Engine::Vector3 vPos, vVel;
            float fAge = 0.f, fLife = 0.f, fSize0 = 0.f, fSize1 = 0.f;
            std::shared_ptr<Engine::Texture> pRamp;
            bool bActive = false;
        };
        struct Spark
        {
            Engine::Vector3 vPos, vVel;
            float fAge = 0.f, fLife = 0.f, fSize = 0.f;
            int   iShape = 0;          // 0 = star, 1 = diamond
            bool  bActive = false;
        };
        struct Ring
        {
            Engine::Vector3 vPos;
            float fAge = 0.f, fLife = 0.f, fSize0 = 0.f, fSize1 = 0.f;
            bool  bActive = false;
        };

        // Matches the "BeamVtx" input layout (pos@0 / uv@12 / color@20, 36B).
        // color.rgb = tint (HDR for sparkles), color.a = lifetime t (ramp U).
        struct DeathVertex { float px, py, pz; float u, v; float r, g, b, a; };

        void BuildQuad(std::vector<DeathVertex>& out,
                       const Engine::Vector3& vCentre, float fHalf,
                       const Engine::Vector3& vRight, const Engine::Vector3& vUp,
                       float tintR, float tintG, float tintB, float fLifeT);

        std::vector<Puff>  m_puffs;
        std::vector<Spark> m_sparks;
        std::vector<Ring>  m_rings;
        int m_iNextPuff = 0, m_iNextSpark = 0, m_iNextRing = 0;
        unsigned int m_uSeed = 1u;
        bool m_bInit = false;

        std::vector<DeathVertex> m_vStar, m_vDiamond, m_vRing, m_vPuff;  // scratch

        std::unordered_map<unsigned int, std::shared_ptr<Engine::Texture>> m_puffRamps;
        std::shared_ptr<Engine::Texture> m_pSparkRamp;
        std::shared_ptr<Engine::Texture> m_pSmokeRamp;

        std::shared_ptr<Engine::Transform>         m_pVP;
        std::shared_ptr<Engine::VertexShader>      m_pVS;
        std::shared_ptr<Engine::PixelShader>       m_pPuffPS;
        std::shared_ptr<Engine::PixelShader>       m_pStarPS;
        std::shared_ptr<Engine::PixelShader>       m_pDiamondPS;
        std::shared_ptr<Engine::PixelShader>       m_pRingPS;
        std::shared_ptr<Engine::InputLayout>       m_pIL;
        std::shared_ptr<Engine::Topology>          m_pTopo;
        std::shared_ptr<Engine::RasterizerState>   m_pNoCull;
        std::shared_ptr<Engine::BlendState>        m_pAlphaBlend;
        std::shared_ptr<Engine::BlendState>        m_pAddBlend;
        std::shared_ptr<Engine::DepthStencilState> m_pNoDepthWrite;
    };
}

#pragma once
#include "Vector3.h"
#include "Types.h"        // UITINTBUFFER / UICBUFFER
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>

namespace Engine
{
    class Texture;
    class Transform;
    class VertexShader;
    class PixelShader;
    class Mesh;
    class Topology;
    class RasterizerState;
    template <typename T> class ConstantBuffer;
}

namespace Client
{
    // Pre-spawn warning circles for enemies. EnemySpawner submits one entry per
    // pending spawn every frame; Render draws a red ring at the full spawn
    // radius plus a filled disc scaled by fFill01 (0 = invisible dot at centre,
    // 1 = disc reaches the ring). When fFill01 hits 1 the spawner materialises
    // the enemy.
    //
    // Submit-per-frame batch (no internal lifetime) — same shape as
    // HealAuraManager. Reuses the registered UI shaders (UIVS + PS_UITint) and
    // a NORMAL-camera Transform for world WVP.
    class SpawnTelegraphManager
    {
        static SpawnTelegraphManager* m_pInst;

    public:
        static SpawnTelegraphManager* GetInst()
        {
            if (!m_pInst) m_pInst = dbg_new SpawnTelegraphManager;
            return m_pInst;
        }
        static void DestroyInst()
        {
            if (m_pInst) { delete m_pInst; m_pInst = nullptr; }
        }

        // Build both ring + disc textures and the shared render resources.
        // Idempotent.
        bool Init();

        // Drop last frame's submissions. Call at the top of the scene Update
        // BEFORE EnemySpawner::Tick runs (which is what submits).
        void BeginFrame() { m_subs.clear(); }

        // Queue a telegraph: red ring at vCentre with radius fRadius, with a
        // filled disc whose radius is fFill01 * fRadius. fFill01 is the
        // spawn-progress ratio in [0, 1].
        void Submit(const Engine::Vector3& vCentre, float fRadius, float fFill01);

        // Draw every queued telegraph (ring + fill). Call from a
        // RENDER_LAYER::ALPHA custom render callback.
        void Render();

    private:
        SpawnTelegraphManager() = default;
        ~SpawnTelegraphManager() = default;
        SpawnTelegraphManager(const SpawnTelegraphManager&) = delete;
        SpawnTelegraphManager& operator=(const SpawnTelegraphManager&) = delete;

        std::shared_ptr<Engine::Texture> EnsureRingTexture();
        std::shared_ptr<Engine::Texture> EnsureDiscTexture();

        struct Tele
        {
            Engine::Vector3 vCentre;
            float           fRadius = 1.f;
            float           fFill01 = 0.f;
        };
        std::vector<Tele> m_subs;
        bool m_bInit = false;

        std::shared_ptr<Engine::Texture>       m_pRingTex;
        std::shared_ptr<Engine::Texture>       m_pDiscTex;
        std::shared_ptr<Engine::Transform>     m_pQuad;
        std::shared_ptr<Engine::VertexShader>  m_pVS;
        std::shared_ptr<Engine::PixelShader>   m_pPS;
        std::shared_ptr<Engine::Mesh>          m_pMesh;
        std::shared_ptr<Engine::Topology>      m_pTopo;
        std::shared_ptr<Engine::RasterizerState> m_pNoCull;
        std::shared_ptr<Engine::ConstantBuffer<Engine::UITINTBUFFER>> m_pTint;
        std::shared_ptr<Engine::ConstantBuffer<Engine::UICBUFFER>>    m_pUI;
    };
}

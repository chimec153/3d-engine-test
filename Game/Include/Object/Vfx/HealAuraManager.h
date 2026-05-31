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
    // Ground "heal aura" circles for heal towers. Each frame a heal tower
    // Submits its (centre, radius) — the radius grows with the heal charge so
    // the disc visibly fills up before a pulse. Render draws them all in one
    // RENDER_LAYER::ALPHA custom-render callback, reusing the same world-
    // alpha-quad technique as FootstepManager (UIVS unit quad laid flat +
    // PS_UITint, tint.a * texture.a = the circle shape's opacity).
    //
    // Singleton like FootstepManager so heal towers (submit) and the renderer
    // (draw) share one per-frame batch without threading a pointer.
    class HealAuraManager
    {
        static HealAuraManager* m_pInst;

    public:
        static HealAuraManager* GetInst()
        {
            if (!m_pInst) m_pInst = dbg_new HealAuraManager;
            return m_pInst;
        }
        static void DestroyInst()
        {
            if (m_pInst) { delete m_pInst; m_pInst = nullptr; }
        }

        // Build the circle texture + shared render resources. Idempotent.
        bool Init();

        // Drop last frame's submissions — call once at the top of the scene
        // Update, BEFORE heal towers run their Update and Submit.
        void BeginFrame() { m_subs.clear(); }

        // Queue a ground circle of world radius fRadius at vCentre, drawn at
        // fAlpha opacity. Called by each heal tower every frame.
        void Submit(const Engine::Vector3& vCentre, float fRadius, float fAlpha);

        // Draw every queued circle. Call from a RENDER_LAYER::ALPHA custom
        // render callback (AlphaBlend already bound there).
        void Render();

    private:
        HealAuraManager() = default;
        ~HealAuraManager() = default;
        HealAuraManager(const HealAuraManager&) = delete;
        HealAuraManager& operator=(const HealAuraManager&) = delete;

        std::shared_ptr<Engine::Texture> EnsureCircleTexture();

        struct Aura
        {
            Engine::Vector3 vCentre;
            float           fRadius = 1.f;
            float           fAlpha  = 0.4f;
        };
        std::vector<Aura> m_subs;
        bool m_bInit = false;

        std::shared_ptr<Engine::Texture>       m_pTex;
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

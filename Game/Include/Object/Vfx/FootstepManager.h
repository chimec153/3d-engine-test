#pragma once
#include "Vector3.h"
#include "Types.h"        // UITINTBUFFER / UICBUFFER / CAMERA_TYPE
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
    // Player footstep marks drawn as flat, alpha-blended quads laid on the
    // ground. The floor is flat, so a true projected decal buys nothing (and
    // the engine's Decal binds only one of the four textures its PS needs).
    // A fixed pool fades each mark out; all of them are drawn in one ALPHA-
    // layer custom render callback, because the alpha pass renders only
    // particles + callbacks (mesh buckets are opaque/deferred only).
    //
    // Singleton like DamageTextManager so Player (spawn) and the renderer
    // (draw) share one pool without threading a pointer.
    //
    // Rendering reuses the registered UI shaders: UIVS multiplies a unit quad
    // by g_matTransform (= world*VP from a NORMAL-camera Transform, i.e. the
    // quad in world space), and PS_UITint outputs tint.rgb with alpha =
    // tint.a * texture.a - so the texture's alpha is the footprint shape, the
    // tint carries colour + the fade. The same quad technique transfers to
    // future boss AoE telegraphs (swap texture / size / colour, drive the
    // lifetime from the skill instead of auto-fading).
    class FootstepManager
    {
        static FootstepManager* m_pInst;

    public:
        static FootstepManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new FootstepManager;
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

        // Build the footprint texture + shared render resources and reset the
        // pool. Idempotent for the resources; always clears the pool.
        bool Init();

        // Drop a footprint at a world position, oriented to a yaw (the player's
        // facing). Recycles the oldest slot when the pool is full.
        void Spawn(const Engine::Vector3& vPos, float fYaw);

        // Age active marks; deactivate once fully faded.
        void Update(float fDeltaTime);

        // Draw every active mark as a ground quad. Call from a RenderManager
        // ALPHA custom-render callback (AlphaBlend is already bound there).
        void Render();

        // Deactivate all marks (scene change).
        void Clear();

    private:
        FootstepManager() = default;
        ~FootstepManager() = default;
        FootstepManager(const FootstepManager&) = delete;
        FootstepManager& operator=(const FootstepManager&) = delete;

        std::shared_ptr<Engine::Texture> EnsureFootTexture();

        struct Step
        {
            Engine::Vector3 vPos;          // foot world position
            float           fYaw    = 0.f; // facing
            float           fAge    = 0.f; // seconds since spawn
            bool            bActive = false;
        };

        std::vector<Step> m_pool;
        int  m_iNext = 0;
        bool m_bInit = false;

        std::shared_ptr<Engine::Texture>       m_pTex;
        std::shared_ptr<Engine::Transform>     m_pQuad;   // reused per draw
        std::shared_ptr<Engine::VertexShader>  m_pVS;
        std::shared_ptr<Engine::PixelShader>   m_pPS;
        std::shared_ptr<Engine::Mesh>          m_pMesh;
        std::shared_ptr<Engine::Topology>      m_pTopo;
        std::shared_ptr<Engine::RasterizerState> m_pNoCull;   // ground quad faces either way
        std::shared_ptr<Engine::ConstantBuffer<Engine::UITINTBUFFER>> m_pTint;
        std::shared_ptr<Engine::ConstantBuffer<Engine::UICBUFFER>>    m_pUI;

        static constexpr int   kPoolSize = 24;
        static constexpr float kLifetime = 4.0f;   // seconds to fully fade
        static constexpr float kSize     = 0.35f;  // quad world size
        static constexpr float kMaxAlpha = 0.55f;  // opacity at spawn
    };
}

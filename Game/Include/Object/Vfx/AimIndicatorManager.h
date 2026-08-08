#pragma once
#include "Vector3.h"
#include "Types.h"        // UITINTBUFFER / UICBUFFER / CAMERA_TYPE
#include "Core/Macro.h"   // dbg_new
#include <memory>

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
    // Target-mode aim indicator: a single flat triangle laid on the ground
    // under the player, its tip pointing at the mouse cursor while LCTRL
    // mouse-aim is active. Driven by Player::Input each frame (Set when aiming,
    // SetVisible(false) otherwise) and drawn in the ALPHA pass.
    //
    // Same ground-quad technique as FootstepManager: the registered UI shaders
    // draw a unit quad in world space (UIVS * g_matTransform from a NORMAL-
    // camera Transform), and PS_UITint outputs tint.rgb with alpha =
    // tint.a * texture.a — so the procedural triangle texture is the shape and
    // the tint carries colour + opacity. Single instance, no pool/fade.
    class AimIndicatorManager
    {
        static AimIndicatorManager* m_pInst;

    public:
        static AimIndicatorManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new AimIndicatorManager;
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

        // Build the triangle texture + shared render resources. Idempotent for
        // the resources; always hides the indicator. Returns false if a shared
        // UI bindable is missing.
        bool Init();

        // Show the indicator at a world position, tip oriented to a yaw (the
        // yaw whose forward points from the player to the cursor).
        void Set(const Engine::Vector3& vPos, float fYaw);

        // Hide the indicator (not aiming / scene change).
        void SetVisible(bool bVisible) { m_bVisible = bVisible; }
        void Clear() { m_bVisible = false; }

        // Draw the indicator if visible. Call from a RenderManager ALPHA
        // custom-render callback (AlphaBlend is already bound there).
        void Render();

    private:
        AimIndicatorManager() = default;
        ~AimIndicatorManager() = default;
        AimIndicatorManager(const AimIndicatorManager&) = delete;
        AimIndicatorManager& operator=(const AimIndicatorManager&) = delete;

        std::shared_ptr<Engine::Texture> EnsureTriangleTexture();

        Engine::Vector3 m_vPos{};
        float           m_fYaw     = 0.f;
        bool            m_bVisible = false;
        bool            m_bInit    = false;

        std::shared_ptr<Engine::Texture>       m_pTex;
        std::shared_ptr<Engine::Transform>     m_pQuad;
        std::shared_ptr<Engine::VertexShader>  m_pVS;
        std::shared_ptr<Engine::PixelShader>   m_pPS;
        std::shared_ptr<Engine::Mesh>          m_pMesh;
        std::shared_ptr<Engine::Topology>      m_pTopo;
        std::shared_ptr<Engine::RasterizerState> m_pNoCull;   // flat quad faces either way
        std::shared_ptr<Engine::ConstantBuffer<Engine::UITINTBUFFER>> m_pTint;
        std::shared_ptr<Engine::ConstantBuffer<Engine::UICBUFFER>>    m_pUI;

        static constexpr float kSize      = 0.6f;   // quad world size (smaller)
        static constexpr float kOrbitDist = 1.5f;   // distance from player centre
    };
}

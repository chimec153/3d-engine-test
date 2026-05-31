#pragma once
#include "Component/MeshRendererComponent.h"

namespace Client
{
    // Game-side MeshRenderer for enemies. Overrides GetInstData to append three
    // per-instance attributes after the engine's 236-byte transform+material
    // block: hit-flash colour (read from the effective material), dissolve time,
    // and burn-rim intensity. Paired with EnemyInst.hlsl (EnemyVSInst /
    // EnemyPSInst) + a 260-byte instance input layout registered by
    // RegisterShaders().
    //
    // This is what lets same-kind enemies collapse into one DrawInstanced call
    // while each still flashes and dissolves independently — previously the
    // per-enemy hit flash (and the PaperBurn sibling) forced solo draws.
    class EnemyMeshRenderer : public Engine::MeshRendererComponent
    {
    public:
        EnemyMeshRenderer() = default;
        EnemyMeshRenderer(const EnemyMeshRenderer& other) = default;
        virtual ~EnemyMeshRenderer() override = default;

        // Game instance-buffer stride: 192 (transform) + 44 (material)
        // + 16 (hit flash float4) + 4 (dissolve time float) + 4 (burn rim float).
        static constexpr int kInstSize = 260;

        // Tags the four game enemy shaders + the instance input layout register
        // under. RenderManager derives the Inst VS/PS names from the solo ones
        // by appending "Inst", so the solo VS/PS must be these exact tags.
        static constexpr const char* kVSTag     = "ClientEnemyVS";
        static constexpr const char* kPSTag     = "ClientEnemyPS";
        static constexpr const char* kVSInstTag = "ClientEnemyVSInst";
        static constexpr const char* kPSInstTag = "ClientEnemyPSInst";

        // Register the enemy shaders + instance input layout once. Idempotent;
        // safe to call from GameScene::Init before any enemy spawns.
        static void RegisterShaders();

        // Per-instance dissolve progress (seconds since death began). Enemy
        // advances this each frame while dying; GetInstData ships it to the
        // EnemyPSInst dissolve. 0 = not dissolving.
        void  SetDissolveTime(float f) { m_fDissolveTime = f; }
        float GetDissolveTime() const  { return m_fDissolveTime; }

        // Per-instance burn-rim intensity (weapon Burn ImpactEffect). Enemy
        // sets this >0 while burning; GetInstData ships it to EnemyPSInst, which
        // turns it into a Fresnel (rim) emissive glow on the silhouette. 0 = off.
        void  SetBurnRim(float f) { m_fBurnRim = f; }
        float GetBurnRim() const  { return m_fBurnRim; }

        virtual void GetInstData(char* pData, int iSize) const override;
        virtual std::shared_ptr<Engine::Component> Clone() override;

    private:
        float m_fDissolveTime = 0.f;
        float m_fBurnRim      = 0.f;
    };
}

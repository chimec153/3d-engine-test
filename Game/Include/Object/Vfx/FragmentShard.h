#pragma once
#include "GameObject/GameObject.h"
#include "Vector3.h"
#include <memory>

namespace Engine
{
    class Transform;
    class Mesh;
    class Material;
    class PaperBurn;
}

namespace Client
{
    class EnemyMeshRenderer;

    // One flying mesh shard of a death shatter. CPU-simulated rigid debris
    // (gravity + drag + spin + floor bounce + ageing), drawn through the shared
    // EnemyMeshRenderer path so it gets the deferred toon shading + per-instance
    // dissolve for free. Self-deactivates when its life ends; the scene's prune
    // pass reaps it. Spawned 12-at-a-time by FragmentShatterManager.
    class FragmentShard : public Engine::GameObject
    {
    public:
        FragmentShard();
        virtual ~FragmentShard() override = default;

        // Set this shard's geometry/material and launch it. Called right after
        // CreateGameObject<FragmentShard> (Init has already built the renderer).
        void Launch(const std::shared_ptr<Engine::Mesh>& pMesh,
                    const std::shared_ptr<Engine::Material>& pMaterial,
                    const Engine::Vector3& vWorldPos,
                    const Engine::Vector3& vVelocity,
                    const Engine::Vector3& vAngularVel,
                    float fScale, float fMaxAge, float fGroundY);

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform> m_pTransform;
        std::shared_ptr<EnemyMeshRenderer> m_pRenderer;
        std::shared_ptr<Engine::PaperBurn> m_pPaperBurn;   // drives the dissolve (b10) so it works solo

        Engine::Vector3 m_vVelocity;
        Engine::Vector3 m_vAngularVel;
        Engine::Vector3 m_vRotation;
        float m_fAge     = 0.f;
        float m_fMaxAge  = 1.6f;
        float m_fGroundY = 0.f;
    };
}

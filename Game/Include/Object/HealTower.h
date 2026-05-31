#pragma once
#include "GameObject/GameObject.h"
#include "Vector3.h"
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class Mesh;
    class Gauge;
    class Decal;
    class Texture;
}

namespace Client
{
    class Attackable;

    // A healing tower: a cylinder that periodically restores HP to nearby
    // allies (the player + every tower) inside kHealRadius. A ground circle
    // telegraphs the next pulse (HealAuraManager): a static outer ring + an
    // inner disc that grows from the centre to fill it as the pulse charges.
    // Like an attack tower it has HP + an AggroTarget and breaks at 0.
    class HealTower :
        public Engine::GameObject
    {
    public:
        HealTower();
        virtual ~HealTower() override;

        // Place on the centre of voxel cell (cx,cz); base sits on the floor
        // top (y = kWallY).
        void SetCell(int cx, int cz);

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

        // Shared cylinder mesh (built once, cached) — also used by the
        // placement ghost so the preview matches.
        static std::shared_ptr<Engine::Mesh> BuildCylinderMesh();

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Material>              m_pMaterial;
        std::shared_ptr<Attackable>                    m_pAttackable;
        // Same world-anchored HP bar Tower uses — see Tower.h. Vertical
        // offset differs because the cylinder is shorter than the cube.
        std::shared_ptr<Engine::Gauge>                 m_pHpBar;

        // Heal telegraph decals projected on the floor. Ring = static outer
        // boundary; Fill = inner disc scaled by charge progress (0..1).
        std::shared_ptr<Engine::Decal>                 m_pRingDecal;

        float m_fHealAcc = 0.f;   // counts up to kHealInterval, then pulses

        // Heal every ally (AggroTarget holder) within kHealRadius.
        void HealNearbyAllies();
    };
}

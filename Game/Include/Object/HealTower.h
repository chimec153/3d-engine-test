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

        // Acquisition-order slot seq (from the heal reserve at placement). Drives
        // this tower's numbered position in the tower HUD; preserved through death
        // so a re-placed heal tower keeps its slot.
        int  GetSlotSeq() const   { return m_iSlotSeq; }
        void SetSlotSeq(int iSeq)  { m_iSlotSeq = iSeq; }

        // Heal-tower level (1..). Merging two heal towers in the shop raises it;
        // the pulse heals iHealAmount * level. Set at placement from the reserve.
        int  GetLevel() const     { return m_iLevel; }
        void SetLevel(int iLevel);

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

        // A heal pulse restored iAmount HP to THIS heal tower (it has an
        // AggroTarget, so HealNearbyAllies heals it too). Green body pulse +
        // "+N" number, mirroring Tower::OnHealed.
        void OnHealed(int iAmount);

        // Immediate removal for a shop SELL (no death shatter): just leaves the
        // scene. A heal tower owns no separate scene instances (it only pulses),
        // so there's nothing to tear down. The owned-count decrement + refund
        // are handled by the shop, so this does NOT touch TowerManager.
        void Despawn() { InActivate(); }

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

        // Heal stats from towers.csv (TowerDatabase), seeded in Init. Default
        // to 0 and are always overwritten there (from the loaded def, or the
        // GameDefs kHeal* constants when no row is loaded), so Update/heal use
        // the data-driven values.
        int   m_iHealAmount   = 0;
        float m_fHealInterval = 0.f;
        float m_fHealRadius   = 0.f;

        float m_fHealAcc = 0.f;   // counts up to m_fHealInterval, then pulses
        int   m_iSlotSeq = 0;     // acquisition-order slot (HUD numbering)
        int   m_iLevel   = 1;     // heal-tower level (scales the pulse amount)

        // Heal every ally (AggroTarget holder) within kHealRadius.
        void HealNearbyAllies();
    };
}

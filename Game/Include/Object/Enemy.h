#pragma once
#include "GameObject\GameObject.h"
#include "Pathfinder.h"
#include <vector>
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class VoxelWorld;
    class ColliderSphere;
    class Collider;
}

namespace Client
{
    class Attackable;

    // Tower-defense-style enemy that chases a target GameObject (typically the
    // Player). Each path is planned with grid A* on the voxel world and treats
    // entering a solid block as "break it first" (cost += BlockBreakTime). When
    // the target moves, the next plan picks up the target's new cell.
    class Enemy :
        public Engine::GameObject
    {
    public:
        Enemy();
        virtual ~Enemy() override = default;

    public:
        // Two visual variants — both share the same chase/path/collide
        // behaviour, only the mesh + material colour differ. Pick before Init.
        enum class MESH_KIND { BOX, CAPSULE };

    public:
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }
        // 2D world — enemies live on the floor layer; the y of the spawn
        // cell is implicit (Client::kWallY for body positioning).
        void SetSpawnCell(int x, int z);
        void SetTarget(const std::shared_ptr<Engine::GameObject>& pTarget) { m_TargetObj = pTarget; }
        void SetSpeed(float fSpeed) { m_fSpeed = fSpeed; }
        float GetSpeed() const { return m_fSpeed; }
        // Spawn-time HP override (sets both max and current). Damage drops
        // m_iHP; reaching 0 deactivates the enemy GameObject.
        void SetMaxHP(int iHP) { m_iMaxHP = iHP; m_iHP = iHP; }
        int  GetHP() const { return m_iHP; }
        // Safe to call before or after Init: pre-Init it just stores the
        // kind for Init to pick up; post-Init it swaps the mesh + material
        // colour immediately.
        void SetMeshKind(MESH_KIND e);

    private:
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;
        MESH_KIND           m_eMeshKind   = MESH_KIND::BOX;
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Material>              m_pMaterial;

        // Weak ref so the enemy doesn't keep the player alive past scene exit.
        std::weak_ptr<Engine::GameObject> m_TargetObj;

        // 2D occupied cell. Y is fixed at Client::kWallY for transform
        // positioning so we only track xz.
        int m_iCellX = 0, m_iCellZ = 0;

        std::vector<Pathfinder::PathStep> m_Path;
        size_t m_iPathIdx = 0;

        float m_fSpeed         = 2.0f;   // cells per second
        float m_fBreakAccum    = 0.f;    // seconds spent breaking the current target cell

        // Health pool. Bullet collisions decrement m_iHP; 0 deactivates
        // the GameObject so the scene's prune pass removes it next frame.
        int m_iMaxHP = 3;
        int m_iHP    = 3;

        // Body collider so the player's bullets (tag "bullet_body") can
        // hit us. Set up + callback wired in Init.
        std::shared_ptr<Engine::ColliderSphere> m_pCollider;

        // Melee attack — when the target is within m_fAttackRange the
        // cooldown ticks down and triggers Player::OnHitBy on expiry.
        std::shared_ptr<Attackable> m_pAttackable;
        float m_fAttackRange    = 1.5f;
        float m_fAttackCooldown = 1.0f;
        float m_fAttackAcc      = 0.f;

        // World cell the last path was planned against — used to detect when
        // the player has moved enough to warrant a fresh A* run.
        int  m_iPlannedTargetX = 0;
        int  m_iPlannedTargetZ = 0;
        bool m_bHasPlan        = false;

        bool ResolveTargetCell(int& tx, int& tz) const;
        bool RecomputePathTo(int tx, int tz);
        Engine::Vector3 CellCenter(int x, int z) const;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    public:
        // Collider BEGIN callback — handles bullet hits.
        void OnCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };
}

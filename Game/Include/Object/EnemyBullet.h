#pragma once
#include "GameObject\GameObject.h"
#include "Matrix.h"
#include <memory>
#include <deque>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class ColliderSphere;
    class Collider;
    class VoxelWorld;
}

namespace Client
{
    class Attackable;

    // Straight-line enemy projectile used by ranged_kite spitters
    // (enemies.json behavior "ranged_kite", special.type "ranged").
    //
    // Deliberately simpler than the player Bullet:
    //  - no movement strategies (always straight),
    //  - no impact effects (just damages and vanishes),
    //  - no piercing / multiply / split / orbital,
    //  - collides only against the player.
    //
    // The damage path mirrors Enemy's melee: an Attackable child holds the
    // damage range, and Player::CollisionPlayerBodyStay calls OnHitBy with
    // that Attackable so the same Hit/Die state transitions fire as on a
    // contact hit.
    class EnemyBullet : public Engine::GameObject
    {
    public:
        EnemyBullet();
        virtual ~EnemyBullet() override = default;

        // Spitter calls this right after CreateGameObject. Bakes direction,
        // damage range, and the lifetime budget (a far miss should
        // eventually self-destruct rather than fly forever).
        void Configure(const Engine::Vector3& vDir,
                       float fSpeedCellsPerSec,
                       int   iDamage,
                       float fLifetime);

        // Borrow the scene's voxel world so the bullet vanishes on a wall
        // hit (otherwise it phases through into the void at the perimeter).
        void SetVoxelWorld(Engine::VoxelWorld* pWorld) { m_pVoxelWorld = pWorld; }

        virtual bool Init()                  override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;
        std::shared_ptr<Attackable>                    m_pAttackable;

        Engine::Vector3     m_vDir       = { 1.f, 0.f, 0.f };
        float               m_fSpeed     = 4.f;     // cells/sec
        float               m_fLifeAcc   = 0.f;
        float               m_fLifetime  = 3.f;
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;

        // Distance-gated position history for the tracer trail (head = front).
        // Mirrors the player Bullet's deque; cleared in Configure so a recycled
        // bullet doesn't streak a line from its previous spawn point.
        std::deque<Engine::Vector3> m_trail;
    };
}

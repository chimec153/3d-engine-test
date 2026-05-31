#include "EnemyBullet.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::EnemyBullet, EnemyBullet)
#include "Attackable.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "Vfx/TrailRenderManager.h"
#include "Core/Macro.h"
#include "../GameDefs.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    namespace EnemyBullet_detail
    {
        // Shared mesh + material so all spitter shots batch into one
        // instancing bucket (mirrors EnsureOrbMesh / EnsureOrbMaterial).
        std::shared_ptr<Engine::Mesh> EnsureMesh()
        {
            return Engine::MeshPresets::UnitSphere(6, 12);
        }

        std::shared_ptr<Engine::Material> EnsureMaterial()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Material>("EnemyBulletMaterial"))
                return pCached;
            auto pMat = Engine::StaticCreateBindable<Engine::Material>("EnemyBulletMaterial");
            if (!pMat) return Engine::StaticFindBindable<Engine::Material>("EnemyBulletMaterial");
            // Hot orange/red — reads as a hostile blob distinct from the
            // player's bullets (configurable per-weapon, mostly cool tones)
            // and the yellow orb.
            pMat->SetDiffuseColor (1.0f, 0.3f, 0.1f, 1.f);
            pMat->SetEmissiveColor({ 1.0f, 0.2f, 0.05f, 1.f });
            return pMat;
        }
    }

    EnemyBullet::EnemyBullet() = default;

    bool EnemyBullet::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Sphere preset is a radius-0.5 unit sphere; shrink to a ~0.15-cell
        // ball so it reads as a small projectile relative to the enemy body.
        if (m_pTransform) m_pTransform->SetScale(0.3f, 0.3f, 0.3f);

        auto pMat = EnemyBullet_detail::EnsureMaterial();
        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(EnemyBullet_detail::EnsureMesh());
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            if (pMat)
            {
                m_pMeshRenderer->SetMaterial(pMat);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, pMat);
            }
        }

        // BULLET group masking PLAYER and TOWER. Both targets' body colliders
        // include BULLET in their mask, so the existing BEGIN dispatch routes
        // the hit to the target side (Player::CollisionPlayerBodyStay /
        // Tower::OnHitByBullet), which reads the "enemy_bullet" tag below.
        m_pCollider = AddComponent<Engine::ColliderSphere>("enemy_bullet");
        if (m_pCollider)
        {
            m_pCollider->SetRadius(0.15f);
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::BULLET);
            m_pCollider->SetMask (Engine::COLLISION_GROUP::PLAYER
                                | Engine::COLLISION_GROUP::TOWER);
            // No BEGIN callback on the bullet side — the target body collider
            // is the one with the central hit dispatcher.
        }

        // Attackable carries the damage range. Configure overwrites with
        // the spitter's projectileDamage (already scaled by round damage
        // multiplier in the spawner copy of EnemyDef).
        m_pAttackable = AddComponent<Attackable>("attackable_bullet", 1, 0, 0, false, false);

        return true;
    }

    void EnemyBullet::Configure(const Engine::Vector3& vDir,
                                float fSpeedCellsPerSec,
                                int   iDamage,
                                float fLifetime)
    {
        m_vDir      = vDir;
        m_vDir.y    = 0.f;
        const float fLen = std::sqrt(m_vDir.x * m_vDir.x + m_vDir.z * m_vDir.z);
        if (fLen > 1e-6f) { m_vDir.x /= fLen; m_vDir.z /= fLen; }
        m_fSpeed    = fSpeedCellsPerSec;
        m_fLifetime = fLifetime;
        m_fLifeAcc  = 0.f;
        m_trail.clear();   // recycled bullet: drop the previous flight's history
        if (m_pAttackable)
            m_pAttackable->SetAttackRange(iDamage, iDamage);
    }

    void EnemyBullet::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        if (!m_pTransform) return;

        // Straight-line motion in XZ. Y locked to the spawn altitude
        // (spawner placed it at the muzzle / body-centre height).
        Engine::Vector3 vPos = m_pTransform->GetPosition();
        vPos.x += m_vDir.x * m_fSpeed * fDeltaTime;
        vPos.z += m_vDir.z * m_fSpeed * fDeltaTime;
        m_pTransform->SetPosition(vPos);

        // Tracer trail — reuses the player bullet's TrailRenderManager pipeline.
        // Fixed hostile warm colour (matches the orange/red material) so an
        // incoming shot reads as "enemy" and its path telegraphs for dodging,
        // distinct from the player's mostly cool-toned bullets. Submitted before
        // the despawn checks so the streak shows on the frame it dies. Radius is
        // the mesh world half-width (unit sphere 0.5 * scale 0.3).
        {
            const TrailPreset& tp = TrailRenderManager::GetPreset(TrailStyle::Tracer);
            if (m_trail.empty() ||
                (vPos - m_trail.front()).LengthSq() >= tp.fMinDist * tp.fMinDist)
            {
                m_trail.push_front(vPos);
                if (static_cast<int>(m_trail.size()) > tp.iMaxPoints)
                    m_trail.pop_back();
            }
            if (m_trail.size() >= 2)
                TrailRenderManager::GetInst()->Submit(
                    m_trail, Engine::Vector3(1.0f, 0.3f, 0.1f), 0.15f,
                    TrailStyle::Tracer);
        }

        // Lifetime cap so a missed shot eventually self-destructs.
        m_fLifeAcc += fDeltaTime;
        if (m_fLifeAcc >= m_fLifetime) { InActivate(); return; }

        // Voxel-wall vanish — sample the cell directly under the bullet.
        // No reflection (player bullets do that); enemies' shots just stop.
        if (m_pVoxelWorld)
        {
            const int bx = static_cast<int>(std::floor(vPos.x));
            const int bz = static_cast<int>(std::floor(vPos.z));
            if (Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz)))
            {
                InActivate();
                return;
            }
        }
    }
}

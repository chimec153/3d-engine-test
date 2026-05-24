#include "Enemy.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Enemy, Enemy)
#include "Orb.h"
#include "Bullet.h"
#include "Attackable.h"
#include "FlowField.h"
#include "Player.h"
#include "../UI/DamageText.h"
#include "../GameDefs.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
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
#include "Types.h"
#include <cmath>

namespace Client
{
    namespace
    {
        // 0.6 x 0.6 x 0.6 cube centred on (0,0,0) in x/z, 0..0.6 in y.
        // transform.position = (cellX + 0.5, sy, cellZ + 0.5) puts the mesh
        // at the floor-level cell center.
        std::shared_ptr<Engine::Mesh> EnsureEnemyMesh()
        {
            return Engine::MeshPresets::AxisBox(
                Engine::Vector3(-0.3f, 0.0f, -0.3f),
                Engine::Vector3( 0.3f, 0.6f,  0.3f));
        }

        // Capsule variant — same footprint envelope as the box (~0.6 cell
        // span, feet at y=0) so the existing ColliderSphere stays correct.
        // CreateCapsuleVertex builds a radius-0.5 capsule centred at the
        // origin; the fScale=0.5 + fYLift formula puts feet on y=0 with
        // total height ~0.7.
        std::shared_ptr<Engine::Mesh> EnsureCapsuleEnemyMesh()
        {
            const float fCyl   = 0.4f;
            const float fScale = 0.5f;
            const float fYLift = (0.5f + fCyl * 0.5f) * fScale;
            return Engine::MeshPresets::UnitCapsule(6, 12, fCyl, fScale, fYLift);
        }

        // One shared material per variant so all box enemies land in one
        // RenderManager instancing bucket and all capsules in another. With
        // STANDARD_SOLID_PS reading colour from the material ConstantBuffer,
        // a bucket only renders correctly when its members share a material;
        // distinct Tags keep box and capsule buckets apart in the
        // MeshRenderer instance-key hash.
        std::shared_ptr<Engine::Material> EnsureEnemyMaterial(Enemy::MESH_KIND e)
        {
            const char* pTag = (e == Enemy::MESH_KIND::CAPSULE)
                ? "EnemyMaterialCapsule" : "EnemyMaterialBox";
            if (auto pCached = Engine::StaticFindBindable<Engine::Material>(pTag))
                return pCached;

            // StaticFindBindable<Material>("Material") returns nullptr in this
            // build, so the previous Clone-the-base path produced no material
            // at all and every enemy fell through to the mesh-slot default
            // colour. Construct a fresh Material directly via CreateBindable
            // — it default-constructs Material() (which finds its own
            // ConstantBuffer<MATERIAL>) and registers it under our tag.
            auto pMat = Engine::StaticCreateBindable<Engine::Material>(pTag);
            if (!pMat) return Engine::StaticFindBindable<Engine::Material>(pTag);

            if (e == Enemy::MESH_KIND::CAPSULE)
            {
                pMat->SetDiffuseColor (0.1f, 1.0f, 0.2f, 1.f);
                pMat->SetEmissiveColor({ 0.0f, 0.0f, 0.0f, 0.f });
            }
            else
            {
                pMat->SetDiffuseColor (1.0f, 0.1f, 0.1f, 1.f);
                pMat->SetEmissiveColor({ 0.0f, 0.0f, 0.0f, 0.f });
            }
            return pMat;
        }
    }

    Enemy::Enemy() = default;

    bool Enemy::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Variant materials are shared across all enemies of the same kind so
        // the RenderManager instancing bucket's representative material
        // matches every member's colour.
        m_pMaterial = EnsureEnemyMaterial(m_eMeshKind);

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(
                m_eMeshKind == MESH_KIND::CAPSULE ? EnsureCapsuleEnemyMesh() : EnsureEnemyMesh());
            if (m_pMaterial)
            {
                m_pMeshRenderer->SetMaterial(m_pMaterial);
                // Override slot(0,0) explicitly — highest priority in
                // GetEffectiveMaterial, so even if the mesh has a populated
                // slot or something upstream caches the legacy m_pMaterial,
                // the per-variant colour wins.
                m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
            }
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
        }

        // Body collider — same y range (0..0.6) as the mesh, centred on x/z.
        // BEGIN-type callback dispatches bullet hits.
        m_pCollider = AddComponent<Engine::ColliderSphere>("enemy_body");
        if (m_pCollider)
        {
            m_pCollider->SetRadius(0.35f);
            m_pCollider->SetOffset({ 0.f, 0.3f, 0.f });
            // Enemy body collides with player (frogclaw-style melee) and
            // bullets (taking damage). Filtering eliminates the big
            // ENEMY×ENEMY cross product that dominated pair counts as
            // spawn density climbed.
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::ENEMY);
            m_pCollider->SetMask(Engine::COLLISION_GROUP::PLAYER
                               | Engine::COLLISION_GROUP::BULLET);
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Enemy::OnCollision);
        }

        // Melee attack source. Stats are modest — Attackable picks a random
        // value in [min, max] per hit, and Player::OnHitBy routes it through
        // its own Attackable so the existing Hit/Die state transitions fire.
        m_pAttackable = AddComponent<Attackable>("attackable", 1, 1, 2);

        return true;
    }

    void Enemy::OnCollision(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float /*fDeltaTime*/)
    {
        if (!pDest) return;
        // Only react to player bullets. Other colliders (player body,
        // other enemies, future melee hitboxes) pass through.
        if (pDest->GetTag() != "bullet_body") return;

        // Per-weapon damage. The bullet owns its own OnHit handling now
        // (Vanish vs Reflect vs Multiply vs NoChange), so we no longer
        // InActivate it here — that would short-circuit reflect/orbital
        // weapons that need to keep flying after impact.
        int iDmg = 1;
        if (auto* pBulletGO = pDest->GetGameObjectOwner())
            if (auto* pBullet = dynamic_cast<Bullet*>(pBulletGO))
                iDmg = pBullet->GetDamage();

        // Floating combat text — spawn a number at the enemy's head.
        // Pure visual; the pool decides whether there's a slot free.
        if (m_pTransform)
        {
            Engine::Vector3 vSpawn = m_pTransform->GetPosition();
            vSpawn.y += 1.4f;   // above the body collider centre
            // Provisional critical rule: top 20% rolls land as crit.
            // Replace once weapons carry their own crit chance.
            const bool bCritical = (std::rand() % 5) == 0;
            const int iShown = bCritical ? iDmg * 2 : iDmg;
            // Pass this enemy as an accumulation key so consecutive
            // hits within ~0.35s fold into a single growing total
            // instead of stacking pop-ups.
            const uintptr_t hOwner = reinterpret_cast<uintptr_t>(this);
            DamageTextManager::GetInst()->Spawn(vSpawn, iShown, bCritical, hOwner);
        }

        m_iHP -= iDmg;
        if (m_iHP <= 0)
        {
            // Drop a pickup orb at the enemy's current position before
            // deactivating — Scene's prune pass removes the corpse next
            // frame; the orb persists until a PlayerBody touches it.
            if (auto* pScene = GetScene())
            {
                if (auto pLayer = pScene->FindLayer(DEFAULT_LAYER))
                {
                    if (auto pOrb = pScene->CreateGameObject<Orb>("Orb", pLayer))
                    {
                        if (auto pTr = pOrb->GetComponent<Engine::Transform>();
                            pTr && m_pTransform)
                        {
                            Engine::Vector3 vPos = m_pTransform->GetPosition();
                            vPos.y += 0.3f;   // float just above the floor
                            pTr->SetPosition(vPos);
                        }
                    }
                }
            }
            InActivate();
        }
    }

    void Enemy::SetMeshKind(MESH_KIND e)
    {
        m_eMeshKind = e;

        // Swap to the shared material for this variant. Both the material
        // tag and the mesh tag now differ between box and capsule, so the
        // RenderManager instancing bucket key splits cleanly and the
        // bucket's representative material colour matches every member.
        m_pMaterial = EnsureEnemyMaterial(e);

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(
                e == MESH_KIND::CAPSULE ? EnsureCapsuleEnemyMesh() : EnsureEnemyMesh());
            if (m_pMaterial)
            {
                m_pMeshRenderer->SetMaterial(m_pMaterial);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
            }
        }
    }

    void Enemy::ApplyDef(const EnemyDef& def)
    {
        SetMaxHP(def.iMaxHP);
        SetSpeed(def.fSpeed);
        m_fAttackRange    = def.fAttackRange;
        m_fAttackCooldown = def.fAttackCooldown;
        SetMeshKind(def.eKind == EnemyKind::Capsule ? MESH_KIND::CAPSULE : MESH_KIND::BOX);

        // Apply CSV-driven material colour, keyed per-id so two rows
        // sharing the same id share one Material (and one instancing
        // bucket). Different ids with the same colour just get separate
        // Materials — harmless and keeps the key trivially derivable.
        char szMatTag[48];
        std::snprintf(szMatTag, sizeof(szMatTag), "EnemyMaterial_%d", def.iId);
        auto pMat = Engine::StaticFindBindable<Engine::Material>(szMatTag);
        if (!pMat) pMat = Engine::StaticCreateBindable<Engine::Material>(szMatTag);
        if (pMat)
        {
            const float fR = ((def.uColorRGB >> 16) & 0xFF) / 255.f;
            const float fG = ((def.uColorRGB >>  8) & 0xFF) / 255.f;
            const float fB = ((def.uColorRGB      ) & 0xFF) / 255.f;
            pMat->SetDiffuseColor (fR, fG, fB, 1.f);
            pMat->SetEmissiveColor({ 0.f, 0.f, 0.f, 0.f });

            m_pMaterial = pMat;
            if (m_pMeshRenderer)
            {
                m_pMeshRenderer->SetMaterial(pMat);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, pMat);
            }
        }

        // Melee damage range — Attackable already created in Init with
        // legacy defaults; overwrite with the CSV row's values.
        if (m_pAttackable)
            m_pAttackable->SetAttackRange(def.iAttackMin, def.iAttackMax);
    }

    void Enemy::SetSpawnCell(int x, int z)
    {
        m_iCellX = x; m_iCellZ = z;
        if (m_pTransform) m_pTransform->SetPosition(CellCenter(x, z));
        m_fBreakAccum = 0.f;
    }

    Engine::Vector3 Enemy::CellCenter(int x, int z) const
    {
        // 2D world — enemies stand on top of the floor block (y=0 cell
        // occupies y=0..1, so the body sits at y=kWallY).
        return Engine::Vector3(
            static_cast<float>(x) + 0.5f,
            static_cast<float>(kWallY),
            static_cast<float>(z) + 0.5f);
    }

    bool Enemy::ResolveTargetCell(int& tx, int& tz) const
    {
        auto pTarget = m_TargetObj.lock();
        if (!pTarget) return false;
        auto pTransform = pTarget->GetComponent<Engine::Transform>();
        if (!pTransform) return false;
        const Engine::Vector3& vPos = pTransform->GetPosition();
        tx = static_cast<int>(std::floor(vPos.x));
        tz = static_cast<int>(std::floor(vPos.z));
        return true;
    }

    void Enemy::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        if (!m_pVoxelWorld || !m_pTransform) return;

        // Periodic melee — when the target sits within m_fAttackRange the
        // cooldown advances and a hit lands on expiry. Runs before steering
        // so a melee strike fires even when we've already arrived at the
        // target's cell (the early-out below skips movement in that case).
        m_fAttackAcc += fDeltaTime;
        if (m_fAttackAcc >= m_fAttackCooldown && m_pAttackable)
        {
            if (auto pTarget = m_TargetObj.lock())
            {
                if (auto pTargetTr = pTarget->GetComponent<Engine::Transform>())
                {
                    const Engine::Vector3 vDelta =
                        pTargetTr->GetPosition() - m_pTransform->GetPosition();
                    if (vDelta.Length() <= m_fAttackRange)
                    {
                        if (auto* pPlayer = dynamic_cast<Player*>(pTarget.get()))
                            pPlayer->OnHitBy(m_pAttackable.get());
                        m_fAttackAcc = 0.f;
                    }
                }
            }
        }

        int tx, tz;
        if (!ResolveTargetCell(tx, tz)) return;        // lost target — stand still

        // Already in the target's cell: stop. Next frame will recheck and chase
        // again if the target moved.
        if (tx == m_iCellX && tz == m_iCellZ)
        {
            m_fBreakAccum = 0.f;
            return;
        }

        if (!m_pFlowField) return;                     // no field — stand still

        int nextX = 0, nextZ = 0;
        Engine::Vector3 vDir;
        if (!m_pFlowField->Sample(m_iCellX, m_iCellZ, nextX, nextZ, vDir))
        {
            // Outside the field window or unreachable. The spawner rebuilds
            // when the player crosses a cell boundary, so this clears once
            // we're back inside the window.
            return;
        }

        // Validate the next cell against the live voxel state. The field
        // is rebuilt only on goal-cell change, so a wall the player placed
        // or another enemy broke mid-tick may not be in the field yet —
        // re-check here so we always do the right thing this frame.
        const Engine::BlockType bNext = m_pVoxelWorld->GetBlock(nextX, kWallY, nextZ);
        if (Engine::IsSolid(bNext))
        {
            const float fNeeded = Engine::BlockBreakTime(bNext);
            if (fNeeded < 0.f) return;                 // unbreakable — wait for rebuild
            m_fBreakAccum += fDeltaTime;
            if (m_fBreakAccum >= fNeeded)
            {
                m_pVoxelWorld->SetBlock(nextX, kWallY, nextZ, Engine::BlockType::Air);
                m_fBreakAccum = 0.f;
            }
            return;
        }
        m_fBreakAccum = 0.f;

        // Slide along the field direction at fSpeed. Y is fixed (kWallY) so
        // no per-frame surface snap is needed. Movement is continuous (no
        // cell-center snapping) so diagonals look smooth.
        Engine::Vector3 vCur = m_pTransform->GetPosition();
        const float fMove = m_fSpeed * fDeltaTime;
        Engine::Vector3 vNew = vCur + vDir * fMove;
        vNew.y = static_cast<float>(kWallY);
        m_pTransform->SetPosition(vNew);

        m_iCellX = static_cast<int>(std::floor(vNew.x));
        m_iCellZ = static_cast<int>(std::floor(vNew.z));
    }
}

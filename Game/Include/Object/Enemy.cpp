#include "Enemy.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Enemy, Enemy)
#include "Orb.h"
#include "Bullet.h"
#include "Attackable.h"
#include "Player.h"
#include "../GameDefs.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Bindable/Capsule.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    namespace
    {
        // 0.6 x 0.6 x 0.6 cube centered on (0,0,0) in x/z, 0..0.6 in y.
        // transform.position = (cellX + 0.5, sy, cellZ + 0.5) puts the mesh
        // at the floor-level cell center.
        std::shared_ptr<Engine::Mesh> EnsureEnemyMesh()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Mesh>("EnemyMesh"))
                return pCached;

            const Engine::Vector3 lo(-0.3f, 0.0f, -0.3f);
            const Engine::Vector3 hi( 0.3f, 0.6f,  0.3f);

            struct Face { Engine::Vector3 n; Engine::Vector3 v[4]; };
            const Face faces[6] = {
                { { 1.f, 0.f, 0.f}, {{hi.x,hi.y,lo.z},{hi.x,hi.y,hi.z},{hi.x,lo.y,hi.z},{hi.x,lo.y,lo.z}} },
                { {-1.f, 0.f, 0.f}, {{lo.x,hi.y,hi.z},{lo.x,hi.y,lo.z},{lo.x,lo.y,lo.z},{lo.x,lo.y,hi.z}} },
                { { 0.f, 1.f, 0.f}, {{lo.x,hi.y,hi.z},{hi.x,hi.y,hi.z},{hi.x,hi.y,lo.z},{lo.x,hi.y,lo.z}} },
                { { 0.f,-1.f, 0.f}, {{hi.x,lo.y,hi.z},{lo.x,lo.y,hi.z},{lo.x,lo.y,lo.z},{hi.x,lo.y,lo.z}} },
                { { 0.f, 0.f, 1.f}, {{hi.x,hi.y,hi.z},{lo.x,hi.y,hi.z},{lo.x,lo.y,hi.z},{hi.x,lo.y,hi.z}} },
                { { 0.f, 0.f,-1.f}, {{lo.x,hi.y,lo.z},{hi.x,hi.y,lo.z},{hi.x,lo.y,lo.z},{lo.x,lo.y,lo.z}} },
            };
            const DirectX::XMFLOAT2 uv[4] = { {0.f,0.f},{1.f,0.f},{1.f,1.f},{0.f,1.f} };

            std::vector<Engine::VertexStandard> verts; verts.reserve(24);
            std::vector<unsigned int> inds; inds.reserve(36);

            for (int f = 0; f < 6; ++f)
            {
                const unsigned int base = static_cast<unsigned int>(verts.size());
                for (int v = 0; v < 4; ++v)
                {
                    Engine::VertexStandard vs = {};
                    vs.pos = faces[f].v[v];
                    vs.normal = faces[f].n;
                    vs.uv = uv[v];
                    verts.push_back(vs);
                }
                inds.push_back(base + 0); inds.push_back(base + 1); inds.push_back(base + 2);
                inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 3);
            }

            return Engine::StaticCreateBindable<Engine::Mesh>("EnemyMesh", verts, inds);
        }

        // Capsule variant — same footprint envelope as the box (~0.6 cell
        // span, feet at y=0) so the existing ColliderSphere stays correct.
        // CreateCapsuleVertex builds a radius-0.5 capsule centred at the
        // origin; we scale x0.5 (radius -> 0.25) and lift so y=0..0.7.
        std::shared_ptr<Engine::Mesh> EnsureCapsuleEnemyMesh()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Mesh>("EnemyCapsuleMesh"))
                return pCached;

            const int   iRings   = 6;
            const int   iSectors = 12;
            const float fCyl     = 0.4f;
            const float fScale   = 0.5f;

            std::vector<Engine::VertexStandard> verts;
            std::vector<unsigned int>           inds;
            Engine::Capsule::CreateCapsuleVertex<Engine::VertexStandard>(iRings, iSectors, fCyl, verts);
            Engine::Capsule::GetCapsuleVertexNormal<Engine::VertexStandard>(iRings, iSectors, fCyl, verts);
            Engine::Capsule::CreateCapsuleIndex(iRings, iSectors, inds);

            const float fLift = (0.5f + fCyl * 0.5f) * fScale;
            for (auto& v : verts)
            {
                v.pos.x *= fScale;
                v.pos.y = v.pos.y * fScale + fLift;
                v.pos.z *= fScale;
            }

            return Engine::StaticCreateBindable<Engine::Mesh>("EnemyCapsuleMesh", verts, inds);
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

    void Enemy::SetSpawnCell(int x, int z)
    {
        m_iCellX = x; m_iCellZ = z;
        if (m_pTransform) m_pTransform->SetPosition(CellCenter(x, z));
        m_Path.clear();
        m_iPathIdx    = 0;
        m_fBreakAccum = 0.f;
        m_bHasPlan    = false;
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

    bool Enemy::RecomputePathTo(int tx, int tz)
    {
        m_Path.clear();
        m_iPathIdx = 0;
        m_bHasPlan = false;
        if (!m_pVoxelWorld) return false;
        const bool bOk = Pathfinder::FindPath(
            *m_pVoxelWorld,
            m_iCellX, m_iCellZ,
            tx, tz,
            m_fSpeed, 64, m_Path);
        if (bOk)
        {
            m_iPlannedTargetX = tx;
            m_iPlannedTargetZ = tz;
            m_bHasPlan = true;
        }
        return bOk;
    }

    void Enemy::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        if (!m_pVoxelWorld || !m_pTransform) return;

        // Periodic melee — when the target sits within m_fAttackRange the
        // cooldown advances and a hit lands on expiry. Runs before pathing
        // so a melee strike fires even when we've already arrived at the
        // target's cell (the early-out below skips pathing in that case).
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
        if (!ResolveTargetCell(tx, tz))
        {
            // Lost the target — stand still.
            return;
        }

        // Already in the target's cell: stop. Next frame will recheck and chase
        // again if the target moved.
        if (tx == m_iCellX && tz == m_iCellZ)
        {
            m_Path.clear();
            m_iPathIdx = 0;
            m_bHasPlan = false;
            return;
        }

        // Plan / replan when there's no plan, the plan ran out, or the target
        // moved to a different cell since we planned.
        const bool bTargetMoved =
            m_bHasPlan && (tx != m_iPlannedTargetX || tz != m_iPlannedTargetZ);
        if (!m_bHasPlan || m_iPathIdx >= m_Path.size() || bTargetMoved)
        {
            if (!RecomputePathTo(tx, tz) || m_Path.empty()) return;
        }

        Pathfinder::PathStep& step = m_Path[m_iPathIdx];

        // World may have changed since planning — another enemy broke the
        // wall, the player placed a new one, etc. Replan on mismatch.
        const Engine::BlockType nowBlock = m_pVoxelWorld->GetBlock(step.x, kWallY, step.z);
        const bool bSolidNow = Engine::IsSolid(nowBlock);
        if (step.bBreak != bSolidNow)
        {
            RecomputePathTo(tx, tz);
            return;
        }

        if (step.bBreak)
        {
            const float fNeeded = Engine::BlockBreakTime(nowBlock);
            if (fNeeded < 0.f)
            {
                RecomputePathTo(tx, tz);
                return;
            }
            m_fBreakAccum += fDeltaTime;
            if (m_fBreakAccum >= fNeeded)
            {
                m_pVoxelWorld->SetBlock(step.x, kWallY, step.z, Engine::BlockType::Air);
                m_fBreakAccum = 0.f;
                step.bBreak   = false;
            }
            return;
        }

        // Plain movement: slide toward the cell center at fSpeed. Y is
        // fixed (kWallY) so no per-frame surface snap is needed.
        const Engine::Vector3 vTarget = CellCenter(step.x, step.z);
        Engine::Vector3 vCur = m_pTransform->GetPosition();
        Engine::Vector3 vDir = vTarget - vCur;
        const float fDist = vDir.Length();
        const float fMove = m_fSpeed * fDeltaTime;

        if (fMove >= fDist || fDist < 1e-4f)
        {
            m_pTransform->SetPosition(vTarget);
            m_iCellX = step.x;
            m_iCellZ = step.z;
            ++m_iPathIdx;
        }
        else
        {
            vDir.Normalize();
            m_pTransform->SetPosition(vCur + vDir * fMove);
        }
    }
}

#include "Enemy.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Enemy, Enemy)
#include "Orb.h"
#include "Bullet.h"
#include "EnemyBullet.h"
#include "EnemyDatabase.h"
#include "Vfx/VfxManager.h"
#include "Vfx/FragmentShatterManager.h"
#include "Vfx/DeathBurstManager.h"
#include "Attackable.h"
#include "AggroTarget.h"
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
#include "EnemyMeshRenderer.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "Core/Graphics.h"
#include "Bindable/Camera.h"
#include "Render/RenderManager.h"
#include "Types.h"
#include <cmath>

namespace Client
{
    namespace
    {
        // Death-shatter sizing.
        //   Box: box_fragment.mesh is a unit box spanning [-1,1], so 0.35 scales
        //        it down to roughly the ~0.6-wide enemy body.
        //   Capsule: capsule_fragment.mesh is expected to be baked at enemy
        //        scale (Fragment Baker Size≈0.25, Cyl≈0.2), so it ships at 1.0.
        //        Re-tune here if you bake at a different scale.
        constexpr float kShatterScaleBox     = 0.35f;
        constexpr float kShatterScaleCapsule = 1.f;

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

    namespace
    {
        // UE의 Dynamic Material Instance(MID)와 동등. 공유 베이스 머티리얼을
        // 적마다 복제해 hit flash 강도가 인접 적과 분리되도록 함.
        // 트레이드오프: MeshRenderer instance key가 머티리얼 태그로 잡히므로
        // 각 적이 별 버킷에 들어가 RenderManager의 DrawInstanced fast path를
        // 잃음(=enemy당 solo draw). 시각적 정확성을 우선.
        std::shared_ptr<Engine::Material> CloneEnemyMaterial(
            const std::shared_ptr<Engine::Material>& pBase)
        {
            if (!pBase) return nullptr;
            return std::static_pointer_cast<Engine::Material>(pBase->Clone());
        }
    }

    bool Enemy::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<EnemyMeshRenderer>("mesh_renderer");

        // UE MID 패턴: 변종 베이스 머티리얼을 클론해 per-enemy 인스턴스 보유.
        // hit flash 같은 per-instance 파라미터가 같은 변종 옆 적에게 번지지 않음.
        m_pMaterial = CloneEnemyMaterial(EnsureEnemyMaterial(m_eMeshKind));

        // Toon 셰이딩으로 캐릭터/적만 셀쉐이딩 (환경 voxel은 DefaultLit 유지).
        if (m_pMaterial) m_pMaterial->SetShadingModel(Engine::SHADING_MODEL_TOON);

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
            // Game-side enemy shaders (EnemyInst.hlsl). RenderManager derives
            // the instanced VS/PS by appending "Inst" to these tags, so the
            // DrawInstanced fast path reaches EnemyVSInst/EnemyPSInst which read
            // per-instance hit flash + dissolve from the instance stream.
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(EnemyMeshRenderer::kVSTag));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (EnemyMeshRenderer::kPSTag));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

            // UE CustomStencil — 적은 stencil=1 (외곽선 PS에서 g_vOutlineColor).
            m_pMeshRenderer->SetCustomStencil(1);
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
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::STAY,  this, &Enemy::OnFieldStay);
        }

        // Melee attack source. Stats are modest — Attackable picks a random
        // value in [min, max] per hit, and Player::OnHitBy routes it through
        // its own Attackable so the existing Hit/Die state transitions fire.
        // No blood particle, and no PaperBurn sibling: enemies dissolve
        // per-instance via EnemyMeshRenderer so the bucket stays eligible for
        // the DrawInstanced fast path.
        m_pAttackable = AddComponent<Attackable>("attackable", 1, 1, 2, false, false);

        return true;
    }

    void Enemy::OnCollision(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float /*fDeltaTime*/)
    {
        if (!pDest) return;
        // Already dying — ignore further hits so we don't re-drop the orb or
        // restart the dissolve.
        if (m_bDying) return;
        // Only react to player bullets. Other colliders (player body,
        // other enemies, future melee hitboxes) pass through.
        if (pDest->GetTag() != "bullet_body") return;

        // Per-weapon damage. The bullet owns its own OnHit handling now
        // (Vanish vs Reflect vs Multiply vs NoChange / Field), so we no
        // longer InActivate it here — that would short-circuit reflect /
        // orbital weapons that need to keep flying after impact.
        int iDmg = 1;
        // Impact point for the hit spark. Default to the body centre; override
        // with the bullet's own position (the surface contact point) so the
        // spark spawns on the body's edge instead of buried inside the opaque
        // mesh, where depth would occlude small/slow particles.
        Engine::Vector3 vHit = m_pTransform ? m_pTransform->GetPosition() : Engine::Vector3();
        vHit.y += 0.4f;
        if (auto* pBulletGO = pDest->GetGameObjectOwner())
            if (auto* pBullet = dynamic_cast<Bullet*>(pBulletGO))
            {
                // A Multiply split child ignores the enemy that spawned it —
                // skip damage (and the hit-flash / combat text below) so the
                // first-hit enemy isn't struck again by its own fragments.
                if (pBullet->IsIgnoring(this)) return;
                // Ticking weapons (Field zones + Sustained orbits / auras /
                // beams) deal damage over time via OnFieldStay, not a one-shot
                // on entry. Prime the tick accumulator so the first tick lands
                // almost immediately, then let STAY take over.
                if (pBullet->TicksDamage())
                {
                    m_fFieldDamageAcc = pBullet->GetTickInterval();
                    return;
                }
                iDmg = pBullet->GetDamage();
                if (auto pBulletTr = pBullet->GetTransform())
                    vHit = pBulletTr->GetPosition();
            }

        // Bullet impact spark — shared pool, not a per-enemy emitter
        // (VfxManager). Only the bullet-hit path sparks; DoT ticks (Field /
        // Burn) go straight to TakeDamage without this.
        VfxManager::GetInst()->SpawnHit(vHit);

        // Pass the bullet hit position as the damage source so a
        // shieldbearer can arc-check it.
        TakeDamage(iDmg, &vHit);
    }

    void Enemy::OnFieldStay(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float fDeltaTime)
    {
        if (!pDest || m_bDying) return;
        if (pDest->GetTag() != "bullet_body") return;

        auto* pBulletGO = pDest->GetGameObjectOwner();
        if (!pBulletGO) return;
        auto* pBullet = dynamic_cast<Bullet*>(pBulletGO);
        if (!pBullet || !pBullet->TicksDamage()) return;   // only DoT-ticking weapons
        if (pBullet->IsIgnoring(this)) return;

        // Field zone position drives the shield arc check too.
        Engine::Vector3 vSrc;
        bool bHasSrc = false;
        if (auto pBulletTr = pBullet->GetTransform())
        {
            vSrc    = pBulletTr->GetPosition();
            bHasSrc = true;
        }
        // Apply the zone's damage once per tick interval for as long as the
        // enemy overlaps it. The loop covers frame hitches that span more
        // than one interval; it stops early if a tick kills us.
        const float fTick = pBullet->GetTickInterval();
        m_fFieldDamageAcc += fDeltaTime;
        while (m_fFieldDamageAcc >= fTick && !m_bDying)
        {
            m_fFieldDamageAcc -= fTick;
            TakeDamage(pBullet->GetDamage(), bHasSrc ? &vSrc : nullptr);
        }
    }

    void Enemy::TakeDamage(int iDmg, const Engine::Vector3* pSource)
    {
        if (m_bDying || m_bSquishing) return;

        // Front-arc shield (shieldbearer). Only effective when the hit has
        // a known source position AND we have a known target to face. The
        // arc check compares the angle between (target_dir) and (source_dir)
        // — if the source is roughly in the same half-plane as the target,
        // the hit hit the shield and damage is reduced.
        if (m_fShieldArcRad > 0.f && m_fShieldReduction > 0.f && pSource && m_pTransform)
        {
            if (auto pTarget = m_TargetObj.lock())
            {
                if (auto pTargetTr = pTarget->GetComponent<Engine::Transform>())
                {
                    const Engine::Vector3 vMyPos = m_pTransform->GetPosition();
                    Engine::Vector3 vFace = pTargetTr->GetPosition() - vMyPos;
                    Engine::Vector3 vFrom = *pSource - vMyPos;
                    vFace.y = 0.f;
                    vFrom.y = 0.f;
                    const float fFaceLen = vFace.Length();
                    const float fFromLen = vFrom.Length();
                    if (fFaceLen > 1e-4f && fFromLen > 1e-4f)
                    {
                        const float fDot = (vFace.x * vFrom.x + vFace.z * vFrom.z)
                                         / (fFaceLen * fFromLen);
                        // cos(arc) — when the source is within ±m_fShieldArcRad
                        // of the facing direction the dot product clears it.
                        if (fDot >= std::cos(m_fShieldArcRad))
                        {
                            iDmg = static_cast<int>(static_cast<float>(iDmg) * (1.f - m_fShieldReduction) + 0.5f);
                            if (iDmg < 1) iDmg = 1;
                        }
                    }
                }
            }
        }

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

        // UE MID `SetScalarParameterValue("HitFlash", 1.0)` 대응.
        // BasePass PS의 ApplyHitFlash가 baseColor와 흰색을 lerp.
        // Update의 TickHitFlash가 매 프레임 감쇠.
        if (m_pMaterial) m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 1.f, 1.f), 1.f);

        // Squash-and-stretch pop on hit (juice). (Re)start the elastic each
        // hit; Update springs it back to m_fBaseScale. If this blow is fatal
        // the death squish below overrides it (Update checks m_bSquishing
        // first), so it only plays on survivable hits.
        m_bHitSquish = true;
        m_fHitSquish = 0.f;

        m_iHP -= iDmg;
        if (m_iHP <= 0)
        {
            // Stylized death burst at the body — flat-colour puff cloud, star
            // / diamond sparkles, hard-edge smoke ring (DeathBurstManager). The
            // puff palette comes from a per-enemy-colour ramp LUT. Replaces the
            // old VfxManager ember burst; the mesh shatter below still fires.
            if (m_pTransform)
            {
                Engine::Vector3 vDeath = m_pTransform->GetPosition();
                vDeath.y += 0.4f;
                // Use the true base scale, not the live transform — a hit
                // squish may have the body mid-deform on the killing blow.
                const float fScale = m_fBaseScale;
                Engine::Vector3 vCol(1.f, 1.f, 1.f);
                if (m_pMaterial)
                {
                    const Engine::Vector4& c = m_pMaterial->GetMaterial().diffuseColor;
                    vCol = Engine::Vector3(c.x, c.y, c.z);
                }
                DeathBurstManager::GetInst()->SpawnBurst(vDeath, fScale, vCol);

                // Boss death juice — subtle camera shake + a radial screen
                // shockwave centred on the corpse (RenderManager warps the HDR
                // resolve along an expanding ring). Regular enemies skip this
                // so the effect stays special to boss kills.
                if (m_bIsBoss)
                {
                    if (auto pCamera = Engine::Graphics::GetInst()->GetCamera())
                        pCamera->AddTrauma(0.35f);
                    Engine::RenderManager::GetInst()->AddShockwave(vDeath);
                }
            }

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
                        // Per-archetype rewards from enemies.json (goldReward /
                        // xpReward) — Player::CollisionPlayerBodyStay reads
                        // these on pickup so a brute kill is meaningfully more
                        // valuable than a swarmling kill.
                        pOrb->SetMoneyValue(m_iGoldReward);
                        pOrb->SetExpValue  (m_iXpReward);
                    }
                }
            }
            // Split-on-death (splitter): spawn iSplitCount children of
            // strSplitId in a tight ring around the corpse before we
            // deactivate. The minions inherit no multipliers — they're
            // the data-baseline of the spawn-id archetype.
            if (m_iSplitCount > 0 && !m_strSplitId.empty())
                SpawnMinions(m_strSplitId, m_iSplitCount, 0.6f);
            // Enter the death squish: the body flattens/widens for kSquishTime,
            // then ShatterBody() bursts it (driven from Update). Size the curve
            // off the true base scale (a hit squish may be mid-deform) and drop
            // the hit squish so the two don't fight.
            m_fDeathBaseScale = m_fBaseScale;
            m_bHitSquish = false;
            m_bSquishing = true;
            m_fSquish    = 0.f;
        }
    }

    void Enemy::ShatterBody()
    {
        if (!m_pTransform) return;
        // Shards scale with the enemy's rendered body size (the cached pre-squish
        // uniform scale, derived from the JSON hitboxRadius), so a big monster
        // bursts into big shards and a small one into small.
        const float fEnemyScale = m_fDeathBaseScale;
        const Engine::Vector3 vFeet = m_pTransform->GetPosition();
        Engine::Vector3 vBody = vFeet;
        vBody.y += 0.3f * fEnemyScale;   // centre of mass scales with the body
        // Shards rest just above the floor the enemy stood on (feet y), not a
        // global plane — the voxel arena floor sits well above 0. Pass the enemy
        // material so shards inherit its colour + toon.
        const FragmentShatterManager::VARIANT eVar =
            (m_eMeshKind == MESH_KIND::CAPSULE)
                ? FragmentShatterManager::VARIANT::CAPSULE
                : FragmentShatterManager::VARIANT::BOX;
        const float fScale = (m_eMeshKind == MESH_KIND::CAPSULE
            ? kShatterScaleCapsule : kShatterScaleBox) * fEnemyScale;
        FragmentShatterManager::GetInst()->SpawnShatter(
            eVar, vBody, fScale, m_pMaterial, vFeet.y + 0.1f);
    }

    void Enemy::SetMeshKind(MESH_KIND e)
    {
        m_eMeshKind = e;

        // 변종 변경 시에도 per-enemy 머티리얼 인스턴스 유지 (MID 패턴).
        m_pMaterial = CloneEnemyMaterial(EnsureEnemyMaterial(e));
        if (m_pMaterial) m_pMaterial->SetShadingModel(Engine::SHADING_MODEL_TOON);

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
        m_iGoldReward     = def.iGoldReward;
        m_iXpReward       = def.iXpReward;
        m_strName         = def.strName;
        SetMeshKind(def.eKind == EnemyKind::Capsule ? MESH_KIND::CAPSULE : MESH_KIND::BOX);

        // Body size from JSON hitboxRadius (px → cell via kPxPerCell).
        // The presets are authored with a 0.3-cell half-width / 0.35-cell
        // collider radius, so we scale the transform by (target / preset)
        // and rewrite the collider to match — otherwise a brute (0.52
        // collider) hits the same volume as a swarmling (0.20) but draws
        // at preset size, breaking visual-vs-collision parity.
        const float fTargetRadius = def.fHitboxRadiusPx / kPxPerCell;
        const float fPresetHalfW  = 0.3f;
        const float fScale        = fTargetRadius / fPresetHalfW;
        if (m_pTransform) m_pTransform->SetScale(fScale, fScale, fScale);
        m_fBaseScale = fScale;
        if (m_pCollider)
        {
            m_pCollider->SetRadius(fTargetRadius);
            // Mesh y range is 0..0.6; centre at half-height after scaling.
            m_pCollider->SetOffset({ 0.f, 0.3f * fScale, 0.f });
        }

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

            // UE MID 패턴: 베이스(=BindableManager에 등록된 공유본)는 색만 보관,
            // 실제 렌더는 per-enemy 클론에 toon+hit flash 등 인스턴스 상태 보유.
            m_pMaterial = CloneEnemyMaterial(pMat);
            if (m_pMaterial)
            {
                m_pMaterial->SetShadingModel(Engine::SHADING_MODEL_TOON);
                if (m_pMeshRenderer)
                {
                    m_pMeshRenderer->SetMaterial(m_pMaterial);
                    m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
                }
            }
        }

        // Melee damage range — Attackable already created in Init with
        // legacy defaults; overwrite with the CSV row's values.
        if (m_pAttackable)
            m_pAttackable->SetAttackRange(def.iAttackMin, def.iAttackMax);

        // === Phase 2 behavior wiring ===
        m_strBehavior = def.strBehavior;

        // Dash params (cells/sec, cells) — JSON values are px, divide by
        // kPxPerCell once here so the tick loop reads in cell-space.
        m_fDashSpeed       = def.fDashSpeedPx / kPxPerCell;
        m_fDashCooldown    = def.fDashCooldown;
        m_fDashRange       = def.fDashRangePx / kPxPerCell;
        m_fDashTelegraph   = def.fDashTelegraph;
        m_fDashCooldownAcc = 0.f;
        m_eDashState       = DashState::Chase;

        // Ranged-kite params.
        m_fProjSpeed       = def.fProjSpeedPx / kPxPerCell;
        m_iProjDamage      = def.iProjDamage;
        m_fFireCooldown    = def.fFireCooldown;
        m_fFireCooldownAcc = 0.f;
        m_fPreferredRange  = def.fPreferredRangePx / kPxPerCell;

        // Explode params.
        m_fExplodeRadius   = def.fExplodeRadiusPx / kPxPerCell;
        m_iExplodeDamage   = def.iExplodeDamage;
        m_fFuseTime        = def.fFuseTime;
        m_fTriggerRange    = def.fTriggerRangePx / kPxPerCell;
        m_bFuseLit         = false;
        m_fFuseAcc         = 0.f;

        // === Phase 3 specials ===
        m_strSplitId       = def.strSplitId;
        m_iSplitCount      = def.iSplitCount;

        // Shield arc — JSON gives the full-cone degrees; we store the
        // half-arc in radians so the TakeDamage check is one cos compare.
        m_fShieldArcRad    = (def.fShieldArcDegrees * 0.5f) * (PI / 180.f);
        m_fShieldReduction = def.fShieldReduction;

        m_strSummonId      = def.strSummonId;
        m_iSummonCount     = def.iSummonCount;
        m_fSummonCooldown  = def.fSummonCooldown;
        m_fSummonCooldownAcc = m_fSummonCooldown;  // first summon waits a full cooldown

        m_fBlinkCooldown   = def.fBlinkCooldown;
        m_fBlinkDistance   = def.fBlinkDistancePx / kPxPerCell;
        m_fBlinkCooldownAcc = m_fBlinkCooldown;

        // Boss flag + phase list (already-parsed BossPhase rows).
        m_bIsBoss          = def.bIsBoss;
        m_vecPhases        = def.vecPhases;
        m_iCurrentPhase    = -1;
        m_fPhaseSpeedMult  = 1.f;
        m_eAbilityState    = AbilityState::Idle;
        m_fAbilityCdAcc    = 0.f;
        m_fAbilityStateAcc = 0.f;
        m_fAltSummonCdAcc  = 0.f;
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

    void Enemy::PullToward(const Engine::Vector3& vCentre, float fFraction)
    {
        if (!m_pTransform) return;
        if (fFraction < 0.f) fFraction = 0.f;
        if (fFraction > 1.f) fFraction = 1.f;

        Engine::Vector3 vTo = vCentre - m_pTransform->GetPosition();
        vTo.y = 0.f;                       // 2D world — pull stays in XZ
        const float d2 = vTo.LengthSq();
        if (d2 < 1e-6f) return;            // already on the centre
        const float dist = std::sqrt(d2);
        vTo /= dist;

        // Update integrates the impulse with exponential decay, giving a total
        // slide of |impulse| / kImpulseDamping. Solve for the impulse that lands
        // the enemy fFraction of the way to the centre.
        //
        // SET (not +=): a piercing/orbital bullet or a multi-projectile weapon
        // fires the AoE pull on the same enemy several times before the impulse
        // decays. Accumulating would stack past the centre and fling the enemy
        // away. Recomputed from the current position each hit, the pending slide
        // is always <= the remaining distance, so it converges instead.
        m_vImpulse = vTo * (dist * fFraction * kImpulseDamping);
    }

    std::shared_ptr<Engine::GameObject> Enemy::PickAggroTarget(Engine::Layer* pLayer)
    {
        if (!pLayer) return nullptr;
        std::shared_ptr<Engine::GameObject> best;
        int bestAggro = 0;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            auto pAggro = p->GetComponent<AggroTarget>();
            if (!pAggro) continue;
            if (!best || pAggro->GetAggro() > bestAggro)
            {
                best = p;
                bestAggro = pAggro->GetAggro();
            }
        }
        return best;
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

        // Death squish: flatten + widen the body over kSquishTime, then burst it
        // into shards and remove. Skip AI / movement; keep the hit flash decaying
        // (the killing blow pinned it to white). easeOut gives a quick snap.
        if (m_bSquishing)
        {
            if (m_pMaterial) m_pMaterial->TickHitFlash(fDeltaTime);
            m_fSquish += fDeltaTime;
            float t = m_fSquish / kSquishTime;
            if (t > 1.f) t = 1.f;
            const float ease = 1.f - (1.f - t) * (1.f - t);
            const float sy  = 1.f + (kSquishFlatY  - 1.f) * ease;
            const float sxz = 1.f + (kSquishWideXZ - 1.f) * ease;
            if (m_pTransform)
                m_pTransform->SetScale(m_fDeathBaseScale * sxz,
                                       m_fDeathBaseScale * sy,
                                       m_fDeathBaseScale * sxz);
            if (m_fSquish >= kSquishTime)
            {
                ShatterBody();
                InActivate();
            }
            return;
        }

        // Dissolving: drive per-instance PaperTime and deactivate once the
        // shader has fully clipped the body. Skip AI / movement, but KEEP
        // decaying the hit flash — the killing blow set it to full white
        // (TakeDamage), and this branch returns before the TickHitFlash
        // below, so without this the flash stays pinned at 1 and the whole
        // dissolving body renders solid white.
        if (m_bDying)
        {
            if (m_pMaterial) m_pMaterial->TickHitFlash(fDeltaTime);
            m_fDissolve += fDeltaTime;
            if (m_pMeshRenderer) m_pMeshRenderer->SetDissolveTime(m_fDissolve);
            if (m_fDissolve >= kDissolveTime) InActivate();
            return;
        }

        // 히트 플래시 강도 감쇠 — 약 1/6초에 0으로 회귀(SetHitFlash와 짝).
        if (m_pMaterial) m_pMaterial->TickHitFlash(fDeltaTime);

        // Hit squish — brief elastic squash that springs back to the base
        // scale. sin(pi*t) pulses 0→1→0 so the body returns exactly to
        // m_fBaseScale with no drift. Nothing else touches scale on alive
        // frames, so this has exclusive control until it completes.
        if (m_bHitSquish && m_pTransform)
        {
            m_fHitSquish += fDeltaTime;
            float t = m_fHitSquish / kHitSquishTime;
            if (t >= 1.f)
            {
                m_bHitSquish = false;
                m_pTransform->SetScale(m_fBaseScale, m_fBaseScale, m_fBaseScale);
            }
            else
            {
                const float pulse = sinf(3.14159265f * t);   // 0→1→0
                const float sy  = 1.f + (kHitSquishFlatY  - 1.f) * pulse;
                const float sxz = 1.f + (kHitSquishWideXZ - 1.f) * pulse;
                m_pTransform->SetScale(m_fBaseScale * sxz,
                                       m_fBaseScale * sy,
                                       m_fBaseScale * sxz);
            }
        }

        if (!m_pVoxelWorld || !m_pTransform) return;

        // Knockback / gather impulse (from weapon ImpactEffects). Integrate
        // before the chase steering so a struck enemy is shoved and then
        // resumes pathing from where it lands. A solid-cell check stops a
        // shove from pushing the body inside a wall.
        if (m_vImpulse.LengthSq() > kImpulseStopSq)
        {
            const Engine::Vector3 vCur = m_pTransform->GetPosition();
            Engine::Vector3 vTry = vCur + m_vImpulse * fDeltaTime;
            const int bx = static_cast<int>(std::floor(vTry.x));
            const int bz = static_cast<int>(std::floor(vTry.z));
            if (!Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz)))
            {
                vTry.y = static_cast<float>(kWallY);
                m_pTransform->SetPosition(vTry);
                m_iCellX = bx;
                m_iCellZ = bz;
            }
            float fDecay = 1.f - kImpulseDamping * fDeltaTime;
            if (fDecay < 0.f) fDecay = 0.f;
            m_vImpulse = m_vImpulse * fDecay;
        }
        else
        {
            m_vImpulse = 0.f;
        }

        // Slow status (weapon Slow ImpactEffect): m_fSlowFactor (<=1) scales the
        // chase speed below for the duration, then resets to 1. A cool-blue
        // hit-flash tint marks the state. Burn now uses its own rim channel
        // (not the flash tint), so a slowed + burning enemy shows both at once.
        if (m_fSlowRemaining > 0.f)
        {
            if (m_pMaterial) m_pMaterial->SetHitFlash(Engine::Vector3(0.3f, 0.6f, 1.f), 0.4f);
            m_fSlowRemaining -= fDeltaTime;
            if (m_fSlowRemaining <= 0.f) { m_fSlowRemaining = 0.f; m_fSlowFactor = 1.f; }
        }

        // Burning status (weapon Burn ImpactEffect): self-timed DoT, same tick
        // shape as the Field zone above but it persists after the bullet is
        // gone. Drive a Fresnel rim glow (per-instance BurnRim → EnemyPSInst)
        // while burning so the state reads as a fiery silhouette rather than a
        // flat tint. It rides its own instance channel, independent of the
        // hit-flash tint — so the white damage flash on each tick still shows.
        if (m_fBurnRemaining > 0.f)
        {
            if (m_pMeshRenderer) m_pMeshRenderer->SetBurnRim(1.f);

            // Flame VFX — small bursts on a fast sub-tick (shared VfxManager
            // pool), repositioned to the body each time so they trail the
            // moving enemy. Decoupled from the damage tick below.
            m_fBurnVfxAcc += fDeltaTime;
            while (m_fBurnVfxAcc >= kBurnVfxInterval)
            {
                m_fBurnVfxAcc -= kBurnVfxInterval;
                if (m_pTransform)
                {
                    Engine::Vector3 vFire = m_pTransform->GetPosition();
                    vFire.y += 0.3f;   // around the body, not the feet
                    VfxManager::GetInst()->SpawnBurn(vFire);
                }
            }

            m_fBurnRemaining -= fDeltaTime;
            m_fBurnTickAcc   += fDeltaTime;
            while (m_fBurnTickAcc >= kBurnTickInterval && !m_bDying)
            {
                m_fBurnTickAcc -= kBurnTickInterval;
                TakeDamage(m_iBurnDamage);
            }
            if (m_fBurnRemaining <= 0.f)
            {
                m_fBurnRemaining = 0.f; m_iBurnDamage = 0; m_fBurnTickAcc = 0.f; m_fBurnVfxAcc = 0.f;
                if (m_pMeshRenderer) m_pMeshRenderer->SetBurnRim(0.f);   // burn ended — kill the rim
            }
            if (m_bDying) return;   // a tick killed us — skip melee / movement this frame
        }

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
                        else if (auto pTA = pTarget->GetComponent<Attackable>())
                            m_pAttackable->Attack(pTA.get());   // tower — self-destructs in its Update
                        m_fAttackAcc = 0.f;
                    }
                }
            }
        }

        int tx, tz;
        if (!ResolveTargetCell(tx, tz)) return;        // lost target — stand still

        // === Phase 3 boss phase driver ===
        // Bosses dispatch first because the phase ability (charge / slam
        // / barrage / summon) preempts the behavior dispatch below.
        // TickBossPhase returns true when the ability owns this frame's
        // movement (or deliberately froze it during a telegraph).
        if (m_bIsBoss)
        {
            if (TickBossPhase(fDeltaTime)) return;
        }

        // === Phase 2 behavior dispatch ===
        // The tick functions return true when they fully handled this
        // frame's movement (or, in the explode case, deliberately froze
        // it). The default chase logic below only runs for "chase"
        // behavior with no active fuse.
        if (m_strBehavior == "dash" && m_fDashSpeed > 0.f)
        {
            if (TickDash(fDeltaTime)) return;
        }
        else if (m_strBehavior == "ranged_kite")
        {
            if (TickRangedKite(fDeltaTime)) return;
        }
        else if (m_fExplodeRadius > 0.f)
        {
            // Chase + explode special (bomber): freeze chase once the fuse
            // is lit. TickExplode returns true while the fuse is in
            // progress so the chase block below is skipped.
            if (TickExplode(fDeltaTime)) return;
        }

        // Blink modifier (phantom). Counts cooldown + does the teleport
        // when ready, then falls through to the chase below so movement
        // continues from the new position.
        if (m_fBlinkCooldown > 0.f && m_fBlinkDistance > 0.f)
            TickBlink(fDeltaTime);

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

        // Tower-occupied cell ahead — treat it like an unbreakable wall and
        // hold position. Dijkstra still records a direction toward a tower
        // GOAL (so adjacent enemies are facing it for melee, and the attack
        // tick above lands the hit), but we never step onto the tower itself.
        if (m_pFlowField->IsBlocked(nextX, nextZ))
        {
            m_fBreakAccum = 0.f;
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
        // m_fPhaseSpeedMult defaults to 1 so non-bosses see no change;
        // boss phases (e.g. devourer "광폭" 1.3×) accelerate the chase.
        const float fMove = m_fSpeed * m_fSlowFactor * m_fPhaseSpeedMult * fDeltaTime;
        Engine::Vector3 vNew = vCur + vDir * fMove;
        vNew.y = static_cast<float>(kWallY);
        m_pTransform->SetPosition(vNew);

        m_iCellX = static_cast<int>(std::floor(vNew.x));
        m_iCellZ = static_cast<int>(std::floor(vNew.z));
    }

    // ============================================================
    // Phase 2 behavior ticks
    // ============================================================

    bool Enemy::TickDash(float fDeltaTime)
    {
        auto pTarget = m_TargetObj.lock();
        if (!pTarget || !m_pTransform) return false;
        auto pTargetTr = pTarget->GetComponent<Engine::Transform>();
        if (!pTargetTr) return false;

        const Engine::Vector3 vMyPos     = m_pTransform->GetPosition();
        const Engine::Vector3 vTargetPos = pTargetTr->GetPosition();

        switch (m_eDashState)
        {
        case DashState::Chase:
        {
            // Cooldown ticks during chase. Once 0, an in-range target
            // triggers the telegraph.
            if (m_fDashCooldownAcc > 0.f) m_fDashCooldownAcc -= fDeltaTime;

            Engine::Vector3 vTo = vTargetPos - vMyPos;
            vTo.y = 0.f;
            const float fDist = vTo.Length();
            if (m_fDashCooldownAcc <= 0.f && fDist > 0.001f && fDist <= m_fDashRange)
            {
                // Lock direction NOW (player movement during the telegraph
                // window is the dasher's whiff opportunity), enter pause.
                m_vDashDir = vTo;
                m_vDashDir.y = 0.f;
                const float fLen = std::sqrt(
                    m_vDashDir.x * m_vDashDir.x + m_vDashDir.z * m_vDashDir.z);
                if (fLen > 1e-4f) { m_vDashDir.x /= fLen; m_vDashDir.z /= fLen; }
                m_eDashState      = DashState::Telegraph;
                m_fDashStateTimer = m_fDashTelegraph;
                // Magenta hit-flash tint: cheap, reuses the existing flash
                // channel that Update::TickHitFlash decays naturally if the
                // enemy is interrupted (killed mid-telegraph).
                if (m_pMaterial)
                    m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.2f, 1.f), 1.f);
                return true;   // pause this frame — fall through to chase next frame
            }
            // Not ready / out of range: fall through to the normal chase
            // movement (return false → caller runs the chase block).
            return false;
        }
        case DashState::Telegraph:
        {
            // Keep the warning tint topped up so the flash doesn't decay
            // back to neutral while we wait.
            if (m_pMaterial)
                m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.2f, 1.f), 1.f);
            m_fDashStateTimer -= fDeltaTime;
            if (m_fDashStateTimer <= 0.f)
            {
                m_eDashState = DashState::Dashing;
                // Re-use the same timer as a distance budget. dashRange
                // cells at dashSpeed cells/sec → dashRange/dashSpeed sec.
                m_fDashStateTimer = (m_fDashSpeed > 0.f)
                    ? (m_fDashRange / m_fDashSpeed) : 0.f;
            }
            return true;
        }
        case DashState::Dashing:
        {
            // Move in the locked direction at dashSpeed. Stop on wall
            // contact OR distance-budget exhaustion.
            const Engine::Vector3 vStep = m_vDashDir * (m_fDashSpeed * fDeltaTime);
            Engine::Vector3 vNew = vMyPos + vStep;
            vNew.y = static_cast<float>(kWallY);
            const int bx = static_cast<int>(std::floor(vNew.x));
            const int bz = static_cast<int>(std::floor(vNew.z));
            bool bHitWall = false;
            if (m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz)))
                bHitWall = true;
            if (!bHitWall)
            {
                m_pTransform->SetPosition(vNew);
                m_iCellX = bx;
                m_iCellZ = bz;
            }
            m_fDashStateTimer -= fDeltaTime;
            if (bHitWall || m_fDashStateTimer <= 0.f)
            {
                m_eDashState       = DashState::Chase;
                m_fDashCooldownAcc = m_fDashCooldown;
            }
            return true;
        }
        }
        return false;
    }

    bool Enemy::TickRangedKite(float fDeltaTime)
    {
        auto pTarget = m_TargetObj.lock();
        if (!pTarget || !m_pTransform) return false;
        auto pTargetTr = pTarget->GetComponent<Engine::Transform>();
        if (!pTargetTr) return false;

        // Fire cadence runs regardless of how the movement chooses to step
        // (so a kiter cornered against a wall still threatens damage).
        if (m_iProjDamage > 0 && m_fFireCooldown > 0.f && m_fProjSpeed > 0.f)
        {
            m_fFireCooldownAcc -= fDeltaTime;
            if (m_fFireCooldownAcc <= 0.f)
            {
                FireProjectileAtTarget();
                m_fFireCooldownAcc = m_fFireCooldown;
            }
        }

        // Summon action runs in parallel with kiting (summoner: drops
        // back to preferredRange while periodically spawning minions).
        TickSummonAction(fDeltaTime);

        const Engine::Vector3 vMyPos     = m_pTransform->GetPosition();
        const Engine::Vector3 vTargetPos = pTargetTr->GetPosition();
        Engine::Vector3 vTo = vTargetPos - vMyPos;
        vTo.y = 0.f;
        const float fDist = vTo.Length();
        if (fDist < 1e-4f) return true;   // standing on top of the target — hold

        Engine::Vector3 vDir = vTo / fDist;   // unit toward target

        // Hysteresis band around the preferred range so the kiter doesn't
        // stutter on the boundary. Retreat if too close, advance if too
        // far, hold inside [0.85*pref, 1.15*pref].
        const float fNear = m_fPreferredRange * 0.85f;
        const float fFar  = m_fPreferredRange * 1.15f;
        Engine::Vector3 vMoveDir;
        if (fDist < fNear)      vMoveDir = vDir * -1.f;
        else if (fDist > fFar)  vMoveDir = vDir;
        else                    return true;   // in sweet spot — hold

        // Wall-rejecting step. No flow field for kiting motion — direct
        // line is fine because the kiter only moves a short hop per frame
        // and the wall check below skips a step that would embed in stone.
        const float fMove = m_fSpeed * m_fSlowFactor * fDeltaTime;
        Engine::Vector3 vNew = vMyPos + vMoveDir * fMove;
        vNew.y = static_cast<float>(kWallY);
        const int bx = static_cast<int>(std::floor(vNew.x));
        const int bz = static_cast<int>(std::floor(vNew.z));
        if (m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz)))
            return true;   // wall ahead — hold rather than wedge in
        m_pTransform->SetPosition(vNew);
        m_iCellX = bx;
        m_iCellZ = bz;
        return true;
    }

    bool Enemy::TickExplode(float fDeltaTime)
    {
        auto pTarget = m_TargetObj.lock();
        if (!pTarget || !m_pTransform) return false;
        auto pTargetTr = pTarget->GetComponent<Engine::Transform>();
        if (!pTargetTr) return false;

        const Engine::Vector3 vMyPos     = m_pTransform->GetPosition();
        const Engine::Vector3 vTargetPos = pTargetTr->GetPosition();
        Engine::Vector3 vTo = vTargetPos - vMyPos;
        vTo.y = 0.f;
        const float fDist = vTo.Length();

        // Light the fuse the first time the target enters trigger range.
        if (!m_bFuseLit && fDist <= m_fTriggerRange)
        {
            m_bFuseLit = true;
            m_fFuseAcc = m_fFuseTime;
        }

        if (!m_bFuseLit) return false;   // chase normally

        // Pulsing red tint as the fuse runs out — value swings between
        // 0.4 and 1.0 ~5 times per second for visual urgency.
        if (m_pMaterial)
        {
            const float fPulse = 0.7f + 0.3f * std::sin(m_fFuseAcc * 30.f);
            m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.f, 0.f), fPulse);
        }
        m_fFuseAcc -= fDeltaTime;
        if (m_fFuseAcc <= 0.f) Detonate();
        return true;   // freeze chase movement while fuse is burning
    }

    void Enemy::FireProjectileAtTarget()
    {
        auto pTarget = m_TargetObj.lock();
        if (!pTarget || !m_pTransform) return;
        auto pTargetTr = pTarget->GetComponent<Engine::Transform>();
        if (!pTargetTr) return;
        auto* pScene = GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const Engine::Vector3 vMyPos     = m_pTransform->GetPosition();
        const Engine::Vector3 vTargetPos = pTargetTr->GetPosition();
        Engine::Vector3 vDir = vTargetPos - vMyPos;
        vDir.y = 0.f;
        if (vDir.LengthSq() < 1e-6f) return;

        auto pBullet = pScene->CreateGameObject<EnemyBullet>("EnemyBullet", pLayer);
        if (!pBullet) return;
        // Spawn at the enemy's body centre (not the feet) so the shot
        // visibly leaves the chest.
        Engine::Vector3 vMuzzle = vMyPos;
        vMuzzle.y += 0.4f;
        if (auto pTr = pBullet->GetComponent<Engine::Transform>())
            pTr->SetPosition(vMuzzle);
        pBullet->SetVoxelWorld(m_pVoxelWorld);
        // Lifetime sized for ~4× preferredRange so a missed shot still
        // self-destructs before reaching the world edge.
        const float fLifetime = (m_fProjSpeed > 0.f && m_fPreferredRange > 0.f)
            ? (m_fPreferredRange * 4.f / m_fProjSpeed) : 3.f;
        pBullet->Configure(vDir, m_fProjSpeed, m_iProjDamage, fLifetime);
    }

    void Enemy::Detonate()
    {
        if (!m_pTransform || !GetScene()) { InActivate(); return; }
        auto pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
        if (!pLayer) { InActivate(); return; }

        const Engine::Vector3 vMyPos = m_pTransform->GetPosition();
        // Big death burst at the body to read as an explosion. Reusing the
        // shared VfxManager death pool keeps this cheap (no per-bomber
        // emitter), but a single SpawnDeath is undersized for an AoE —
        // submit a small cluster offset around the centre.
        for (int i = 0; i < 5; ++i)
        {
            Engine::Vector3 vJitter = vMyPos;
            vJitter.x += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 0.6f;
            vJitter.z += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 0.6f;
            vJitter.y += 0.4f;
            VfxManager::GetInst()->SpawnDeath(vJitter);
        }

        // AoE: walk every active GameObject in the default layer, apply
        // damage to anything inside fExplodeRadius. Player goes through
        // the Attackable path (so Hit/Die state transitions fire);
        // enemies route through Enemy::TakeDamage (drops their orb).
        const float fR2 = m_fExplodeRadius * m_fExplodeRadius;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            if (p.get() == this) continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            Engine::Vector3 vDelta = pTr->GetPosition() - vMyPos;
            vDelta.y = 0.f;
            if (vDelta.LengthSq() > fR2) continue;

            // Player branch — go through OnHitBy so PlayerLowerHitState
            // fires (same as a melee hit). Bomber's own m_pAttackable
            // doesn't carry the explosion damage, so build a temporary
            // by stashing the explode value on it: SetAttackRange clamps
            // and Attack reads from it.
            if (auto* pPlayer = dynamic_cast<Player*>(p.get()))
            {
                if (m_pAttackable)
                {
                    m_pAttackable->SetAttackRange(m_iExplodeDamage, m_iExplodeDamage);
                    pPlayer->OnHitBy(m_pAttackable.get());
                }
                continue;
            }
            // Enemy branch — friendly-fire so a clustered swarm gets
            // shredded along with the bomber. Skips other bombers'
            // fuses (they take HP damage like anyone else).
            if (p->GetTag() == "Enemy")
            {
                if (auto pEnemy = std::dynamic_pointer_cast<Enemy>(p))
                    pEnemy->TakeDamage(m_iExplodeDamage, &vMyPos);
            }
        }

        // Bomber dies with the explosion. Route through TakeDamage so the
        // normal death path runs — orb drop (gold/xp reward), shatter VFX,
        // and InActivate. m_bDying is never set in this codebase so this
        // doesn't short-circuit; the m_iHP <= 0 branch fires once.
        TakeDamage((std::max)(m_iHP, 1));
    }

    // ============================================================
    // Phase 3 ticks + helpers
    // ============================================================

    bool Enemy::TickBlink(float fDeltaTime)
    {
        m_fBlinkCooldownAcc -= fDeltaTime;
        if (m_fBlinkCooldownAcc > 0.f) return false;

        auto pTarget = m_TargetObj.lock();
        if (!pTarget || !m_pTransform) { m_fBlinkCooldownAcc = m_fBlinkCooldown; return false; }
        auto pTargetTr = pTarget->GetComponent<Engine::Transform>();
        if (!pTargetTr) { m_fBlinkCooldownAcc = m_fBlinkCooldown; return false; }

        const Engine::Vector3 vMyPos = m_pTransform->GetPosition();
        Engine::Vector3 vTo = pTargetTr->GetPosition() - vMyPos;
        vTo.y = 0.f;
        const float fDist = vTo.Length();
        m_fBlinkCooldownAcc = m_fBlinkCooldown;   // reset whether or not we teleport
        if (fDist < 1e-4f) return false;
        const float fStep = (std::min)(m_fBlinkDistance, fDist - 0.5f);   // never blink ONTO the target
        if (fStep <= 0.f) return false;
        Engine::Vector3 vDir = vTo / fDist;

        Engine::Vector3 vNew = vMyPos + vDir * fStep;
        vNew.y = static_cast<float>(kWallY);
        const int bx = static_cast<int>(std::floor(vNew.x));
        const int bz = static_cast<int>(std::floor(vNew.z));
        // If the destination is solid, don't blink into the wall — skip
        // this tick and try again next cooldown.
        if (m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz)))
            return false;
        m_pTransform->SetPosition(vNew);
        m_iCellX = bx;
        m_iCellZ = bz;

        // Brief white flash so the teleport reads visually rather than as
        // a one-frame snap that the eye misses.
        if (m_pMaterial)
            m_pMaterial->SetHitFlash(Engine::Vector3(0.7f, 0.7f, 1.f), 0.8f);
        return true;
    }

    void Enemy::TickSummonAction(float fDeltaTime)
    {
        if (m_iSummonCount <= 0 || m_strSummonId.empty()) return;
        if (m_fSummonCooldown <= 0.f)                     return;

        m_fSummonCooldownAcc -= fDeltaTime;
        if (m_fSummonCooldownAcc > 0.f) return;
        SpawnMinions(m_strSummonId, m_iSummonCount, 1.5f);
        m_fSummonCooldownAcc = m_fSummonCooldown;
    }

    void Enemy::SpawnMinions(const std::string& strId, int iCount, float fRingRadius)
    {
        if (iCount <= 0 || strId.empty()) return;
        auto* pScene = GetScene();
        if (!pScene || !m_pTransform) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;
        const EnemyDef* pDef = EnemyDatabase::GetInst().Get(strId);
        if (!pDef) return;

        const Engine::Vector3 vMyPos = m_pTransform->GetPosition();
        auto pTarget = m_TargetObj.lock();   // hand spawnees the same target

        for (int i = 0; i < iCount; ++i)
        {
            const float fAngle = (static_cast<float>(i) / iCount) * 2.f * PI;
            const float fX = vMyPos.x + std::cos(fAngle) * fRingRadius;
            const float fZ = vMyPos.z + std::sin(fAngle) * fRingRadius;
            const int   cx = static_cast<int>(std::floor(fX));
            const int   cz = static_cast<int>(std::floor(fZ));
            if (m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(cx, kWallY, cz)))
                continue;   // wall slot — skip

            auto pEnemy = pScene->CreateGameObject<Enemy>("Enemy", pLayer);
            if (!pEnemy) continue;
            pEnemy->ApplyDef(*pDef);
            pEnemy->SetVoxelWorld(m_pVoxelWorld);
            pEnemy->SetFlowField(m_pFlowField);
            pEnemy->SetSpawnCell(cx, cz);
            if (pTarget) pEnemy->SetTarget(pTarget);
        }
    }

    void Enemy::ApplySlamDamage(float fRadius, int iDamage)
    {
        if (!m_pTransform || !GetScene()) return;
        auto pLayer = GetScene()->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const Engine::Vector3 vMyPos = m_pTransform->GetPosition();
        const float fR2 = fRadius * fRadius;
        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            if (p.get() == this) continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            Engine::Vector3 vDelta = pTr->GetPosition() - vMyPos;
            vDelta.y = 0.f;
            if (vDelta.LengthSq() > fR2) continue;

            if (auto* pPlayer = dynamic_cast<Player*>(p.get()))
            {
                if (m_pAttackable)
                {
                    m_pAttackable->SetAttackRange(iDamage, iDamage);
                    pPlayer->OnHitBy(m_pAttackable.get());
                }
            }
        }
        // Burst VFX at slam centre — reuse the shared death pool.
        for (int i = 0; i < 6; ++i)
        {
            Engine::Vector3 vJitter = vMyPos;
            vJitter.x += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * fRadius;
            vJitter.z += (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * fRadius;
            vJitter.y += 0.4f;
            VfxManager::GetInst()->SpawnDeath(vJitter);
        }
    }

    bool Enemy::TickBossPhase(float fDeltaTime)
    {
        if (m_vecPhases.empty() || !m_pTransform) return false;

        // Phase transition — walk top→bottom and pick the highest-listed
        // phase whose hpThreshold still bounds the current HP fraction.
        // JSON authored in descending order (1.0 → 0.66 → 0.33), so the
        // last phase passing the check is the active one.
        const float fHpFrac = (m_iMaxHP > 0)
            ? (static_cast<float>(m_iHP) / static_cast<float>(m_iMaxHP)) : 0.f;
        int iPhase = 0;
        for (size_t i = 0; i < m_vecPhases.size(); ++i)
            if (fHpFrac <= m_vecPhases[i].fHpThreshold) iPhase = static_cast<int>(i);

        if (iPhase != m_iCurrentPhase)
        {
            m_iCurrentPhase    = iPhase;
            m_eAbilityState    = AbilityState::Idle;
            m_fAbilityCdAcc    = 0.f;        // first ability use fires immediately
            m_fAbilityStateAcc = 0.f;
            m_fAltSummonCdAcc  = 0.f;
            m_fPhaseSpeedMult  = m_vecPhases[iPhase].fMoveSpeedMult;
            // Brief flash to signal the phase change.
            if (m_pMaterial)
                m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 1.f, 0.3f), 1.f);
        }

        const BossPhase& ph = m_vecPhases[m_iCurrentPhase];

        // alsoSummon (secondary channel) runs in parallel to the primary
        // ability — independent cooldown, doesn't gate movement.
        if (ph.iAltSummonCount > 0 && ph.fAltSummonCooldown > 0.f && !ph.strAltSummonId.empty())
        {
            m_fAltSummonCdAcc -= fDeltaTime;
            if (m_fAltSummonCdAcc <= 0.f)
            {
                SpawnMinions(ph.strAltSummonId, ph.iAltSummonCount, 2.0f);
                m_fAltSummonCdAcc = ph.fAltSummonCooldown;
            }
        }

        if (ph.eAbility == AbilityType::None) return false;   // chase as usual

        // Drive the ability state machine: Idle (cooldown ticking) →
        // Telegraph (frozen, warning) → Active (one-shot effect) → Idle.
        switch (m_eAbilityState)
        {
        case AbilityState::Idle:
            m_fAbilityCdAcc -= fDeltaTime;
            if (m_fAbilityCdAcc <= 0.f)
            {
                m_eAbilityState    = AbilityState::Telegraph;
                m_fAbilityStateAcc = ph.fTelegraphTime;
                if (m_pMaterial)
                    m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.4f, 0.f), 1.f);
                // Charge locks heading now so the player can sidestep
                // during the telegraph window.
                if (ph.eAbility == AbilityType::Charge)
                {
                    if (auto pTarget = m_TargetObj.lock())
                    {
                        if (auto pTr = pTarget->GetComponent<Engine::Transform>())
                        {
                            const Engine::Vector3 vMy = m_pTransform->GetPosition();
                            Engine::Vector3 vTo = pTr->GetPosition() - vMy;
                            vTo.y = 0.f;
                            const float fLen = vTo.Length();
                            if (fLen > 1e-4f) m_vAbilityDir = vTo / fLen;
                        }
                    }
                }
            }
            return false;   // idle → chase normally

        case AbilityState::Telegraph:
            if (m_pMaterial)
                m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.4f, 0.f), 1.f);
            m_fAbilityStateAcc -= fDeltaTime;
            if (m_fAbilityStateAcc <= 0.f)
            {
                // Telegraph done → activate. Charge needs to last for the
                // dash duration; the others fire-and-forget on entry.
                m_eAbilityState = AbilityState::Active;
                if (ph.eAbility == AbilityType::Charge)
                {
                    const float fChargeSpeed = ph.fChargeSpeedPx / kPxPerCell;
                    // Re-use dashRange's spec convention (range ~= speed *
                    // ~1 sec) since JSON doesn't carry an explicit duration.
                    m_fAbilityStateAcc = (fChargeSpeed > 0.f) ? 1.f : 0.f;
                }
                else
                {
                    // Fire-and-forget — execute once now, then back to Idle.
                    if (ph.eAbility == AbilityType::Slam)
                        ApplySlamDamage(ph.fSlamRadiusPx / kPxPerCell, ph.iSlamDamage);
                    else if (ph.eAbility == AbilityType::Barrage)
                    {
                        const float fSpread = ph.fSpreadDegrees * (PI / 180.f);
                        const float fHalf   = fSpread * 0.5f;
                        // Aim cone centred on the bearing to the target.
                        Engine::Vector3 vAim = { 0.f, 0.f, 1.f };
                        if (auto pTarget = m_TargetObj.lock())
                        {
                            if (auto pTr = pTarget->GetComponent<Engine::Transform>())
                            {
                                Engine::Vector3 vTo = pTr->GetPosition() - m_pTransform->GetPosition();
                                vTo.y = 0.f;
                                const float fLen = vTo.Length();
                                if (fLen > 1e-4f) vAim = vTo / fLen;
                            }
                        }
                        const float fAimAngle = std::atan2(vAim.x, vAim.z);
                        const int iShots = (std::max)(1, ph.iShotsPerVolley);
                        for (int i = 0; i < iShots; ++i)
                        {
                            float fAngle;
                            if (iShots == 1) fAngle = fAimAngle;
                            else
                            {
                                const float fT = static_cast<float>(i) / (iShots - 1);
                                fAngle = fAimAngle - fHalf + fSpread * fT;
                            }
                            const Engine::Vector3 vDir(
                                std::sin(fAngle), 0.f, std::cos(fAngle));
                            // Temporarily project the ability's projectile
                            // params into the regular fire helper — m_fProjSpeed
                            // and m_iProjDamage are saved/restored.
                            const float fSavedSpeed = m_fProjSpeed;
                            const int   iSavedDmg   = m_iProjDamage;
                            m_fProjSpeed  = ph.fProjSpeedPx / kPxPerCell;
                            m_iProjDamage = ph.iProjDamage;
                            // Plumb a "fire in direction" path by spoofing
                            // the target heading. Simpler: spawn directly.
                            auto* pScene = GetScene();
                            auto pLayer  = pScene ? pScene->FindLayer(DEFAULT_LAYER) : nullptr;
                            if (pScene && pLayer)
                            {
                                auto pBullet = pScene->CreateGameObject<EnemyBullet>("EnemyBullet", pLayer);
                                if (pBullet)
                                {
                                    Engine::Vector3 vMuzzle = m_pTransform->GetPosition();
                                    vMuzzle.y += 0.4f;
                                    if (auto pTr = pBullet->GetComponent<Engine::Transform>())
                                        pTr->SetPosition(vMuzzle);
                                    pBullet->SetVoxelWorld(m_pVoxelWorld);
                                    const float fLife = (m_fProjSpeed > 0.f) ? (12.f / m_fProjSpeed) : 3.f;
                                    pBullet->Configure(vDir, m_fProjSpeed, m_iProjDamage, fLife);
                                }
                            }
                            m_fProjSpeed  = fSavedSpeed;
                            m_iProjDamage = iSavedDmg;
                        }
                    }
                    else if (ph.eAbility == AbilityType::Summon)
                    {
                        SpawnMinions(ph.strSummonId, ph.iSummonCount, 2.0f);
                    }
                    m_eAbilityState = AbilityState::Idle;
                    m_fAbilityCdAcc = ph.fAbilityCooldown;
                }
            }
            return true;   // frozen during telegraph

        case AbilityState::Active:
            // Only Charge uses the Active phase (sustained motion). Step
            // forward at the charge speed until the duration runs out OR
            // we hit a wall.
            if (ph.eAbility == AbilityType::Charge)
            {
                if (m_pMaterial)
                    m_pMaterial->SetHitFlash(Engine::Vector3(1.f, 0.f, 0.f), 0.8f);
                const float fChargeSpeed = ph.fChargeSpeedPx / kPxPerCell;
                const Engine::Vector3 vStep = m_vAbilityDir * (fChargeSpeed * fDeltaTime);
                Engine::Vector3 vNew = m_pTransform->GetPosition() + vStep;
                vNew.y = static_cast<float>(kWallY);
                const int bx = static_cast<int>(std::floor(vNew.x));
                const int bz = static_cast<int>(std::floor(vNew.z));
                bool bHitWall = m_pVoxelWorld && Engine::IsSolid(m_pVoxelWorld->GetBlock(bx, kWallY, bz));
                if (!bHitWall)
                {
                    m_pTransform->SetPosition(vNew);
                    m_iCellX = bx;
                    m_iCellZ = bz;
                }
                m_fAbilityStateAcc -= fDeltaTime;
                if (bHitWall || m_fAbilityStateAcc <= 0.f)
                {
                    m_eAbilityState = AbilityState::Idle;
                    m_fAbilityCdAcc = ph.fAbilityCooldown;
                }
                return true;
            }
            // Other abilities transition Telegraph → Idle directly above,
            // so we shouldn't normally reach Active for them. Defensive.
            m_eAbilityState = AbilityState::Idle;
            m_fAbilityCdAcc = ph.fAbilityCooldown;
            return false;
        }
        return false;
    }
}

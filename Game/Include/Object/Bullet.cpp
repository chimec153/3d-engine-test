#include "Bullet.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Bullet, Bullet)
#include "WeaponDatabase.h"
#include "Movement/BulletMovementFactory.h"
#include "Movement/IBulletMovement.h"
#include "Movement/EnemyTargeting.h"   // FindTargetEnemy (Chain on-hit)
#include <cmath>                        // atan2f (Chain redirect)
#include "Impact/IImpactEffect.h"
#include "Impact/ImpactEffectFactory.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Collider.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Vfx/TrailRenderManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "Voxel/VoxelWorld.h"
#include <cmath>

namespace Client
{
    namespace Bullet_detail
    {
        // 0xRRGGBB packed → float[4] (0..1) with full alpha.
        inline DirectX::XMFLOAT4 ToFloat4(unsigned int uColorRGB)
        {
            const float r = ((uColorRGB >> 16) & 0xFF) / 255.f;
            const float g = ((uColorRGB >>  8) & 0xFF) / 255.f;
            const float b = ((uColorRGB      ) & 0xFF) / 255.f;
            return { r, g, b, 1.f };
        }
    }

    Bullet::Bullet() :
        m_eOnHit(OnHitEvent::Vanish)
        , m_eFireMode(FireMode::Cooldown)
        , m_iDamage(5)
        , m_fSpeed(8.f)
        , m_fLifetime(2.f)
    {
    }

    // Defined here (not =default in the header) so unique_ptr<IBulletMovement>
    // can be instantiated against a forward declaration in Bullet.h.
    Bullet::~Bullet() = default;

    void Bullet::ScaleDamage(float fMul)
    {
        if (fMul <= 0.f) return;
        int d = static_cast<int>(m_iDamage * fMul + 0.5f);
        m_iDamage = d < 1 ? 1 : d;
    }

    bool Bullet::Init()
    {
        if (!__super::Init())
            return false;

        // Components shared by every bullet variant — Configure tweaks the
        // mesh/material/lifetime/movement afterward.
        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Default to a sphere; Configure may swap the mesh for box/triangle.
        ApplyShape(ProjectileShape::Sphere, 0xFFCC33);

        if (m_pTransform)
            m_pTransform->SetScale(0.25f, 0.25f, 0.25f);

        m_pCollider = AddComponent<Engine::ColliderSphere>("bullet_body");
        if (m_pCollider)
        {
            // Match the visible sphere: unit mesh radius 0.5 × the 0.25
            // default scale = 0.125 world units. Configure() overrides this
            // per weapon (fSize × 0.5); this is just the pre-Configure value.
            m_pCollider->SetRadius(0.125f);
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::BULLET);
            m_pCollider->SetMask(Engine::COLLISION_GROUP::ENEMY);
            // BEGIN callback — drives the OnHit enum (Vanish / Reflect /
            // Multiply / NoChange). Enemy::OnCollision still applies
            // damage but no longer InActivates the bullet.
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::BEGIN,
                this, &Bullet::OnBeginCollision);
        }

        // The tracer trail is a camera-facing additive ribbon drawn by
        // TrailRenderManager from this bullet's position history (filled in
        // Update, submitted each frame) -- no per-bullet Particle component.

        return true;
    }

    void Bullet::Configure(const WeaponDef& def, int iLevel,
                           std::weak_ptr<Engine::Transform> pOwner)
    {
        m_eOnHit    = def.eOnHit;
        m_eFireMode = def.eFireMode;
        m_eMovement = def.eMovement;
        m_iWeaponId = def.iId;
        m_iLevel    = iLevel;
        m_pOwner    = pOwner;
        m_iMaxHits  = def.iMaxHits;
        m_iHitCount = 0;
        // DoT cadence, raw: 0 = single hit, >0 = tick on STAY. See TicksDamage.
        m_fTickInterval = def.fDamageInterval;
        m_iSplitDepth = MultiplySplitDepth(def);   // Multiply generations (0 otherwise)

        m_iDamage       = ComputeDamage(def, iLevel);
        m_fSpeed        = ComputeSpeed (def, iLevel);
        m_fAcceleration = def.fAcceleration;
        m_fLifetime     = def.fLifetime;
        m_fLifeAcc      = 0.f;

        ApplyShape(def.eShape, def.uColorRGB);

        // Per-weapon size — drives both the visual scale and the collider
        // radius. The unit sphere mesh has radius 0.5 (verts span ±0.5), so
        // the on-screen radius is 0.5 * fSize; matching the collider to that
        // makes the hit sphere line up with the visible bullet instead of
        // being ~2x too big. ComputeSize folds in the per-level Size bump
        // when the weapon's level_up_field is Size, else returns def.fSize.
        constexpr float kMeshRadius = 0.5f;   // unit sphere mesh radius
        const float fSize = ComputeSize(def, iLevel);
        if (m_pTransform)
            m_pTransform->SetScale(fSize, fSize, fSize);
        if (m_pCollider)
            m_pCollider->SetRadius(fSize * kMeshRadius);
        m_fRadius = fSize * kMeshRadius;   // look-ahead pad for wall reflection

        // Swap in the movement strategy. Orbital's OnAttached seeds the
        // starting angle from the bullet transform yaw, which Player sets
        // per-instance for orb spread; other types ignore it.
        m_pMovement = MakeBulletMovement(def.eMovement, pOwner);
        if (m_pMovement)
        {
            m_pMovement->SetRadius(def.fOrbitRadius);        // orbital only; 0 = follow
            m_pMovement->SetRadialSpeed(def.fRadialSpeed);   // orbital only; >0 spirals out
            m_pMovement->SetAimMode(def.eAimMode);           // Aimed / Homing only

            if (m_pTransform) m_pMovement->OnAttached(*m_pTransform);
        }

        // Weapon impact modules (Knockback / Gather / Burn / Slow). Empty for a
        // Damage-only weapon; each entry runs on every enemy hit in OnBeginCollision.
        m_pImpactEffects = MakeImpactEffects(def);

        // Tracer trail colour + per-weapon style preset. Clear any history
        // left over from a previous life (bullets are pooled and reused).
        auto col = Bullet_detail::ToFloat4(def.uColorRGB);
        m_vColor = Engine::Vector3(col.x, col.y, col.z);
        m_eTrailStyle = def.eTrailStyle;
        m_trail.clear();
    }

    void Bullet::AddImpactEffect(std::unique_ptr<IImpactEffect> pEffect)
    {
        // Layered on top of the weapon's own effects (built in Configure), so a
        // tower's intrinsic effect and the equipped weapon's both run on a hit.
        if (pEffect) m_pImpactEffects.push_back(std::move(pEffect));
    }

    void Bullet::SetOrbitYOffset(float f)
    {
        if (m_pMovement) m_pMovement->SetYOffset(f);
    }

    void Bullet::ApplyShape(ProjectileShape eShape, unsigned int uColorRGB)
    {
        if (!m_pMeshRenderer) return;

        std::shared_ptr<Engine::Mesh> pMesh;
        switch (eShape)
        {
        case ProjectileShape::Box:      pMesh = Engine::MeshPresets::UnitBox();         break;
        case ProjectileShape::Triangle: pMesh = Engine::MeshPresets::UnitTriangle();    break;
        case ProjectileShape::Sphere:
        default:                        pMesh = Engine::MeshPresets::UnitSphere(8, 16); break;
        }

        m_pMeshRenderer->SetMesh(pMesh);
        m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
        m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
        m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
        m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

        if (auto pSrcMat = Engine::StaticFindBindable<Engine::Material>("Material"))
        {
            auto pMat = std::static_pointer_cast<Engine::Material>(pSrcMat->Clone());
            auto c = Bullet_detail::ToFloat4(uColorRGB);
            pMat->SetDiffuseColor (c.x, c.y, c.z, 1.f);
            pMat->SetSpecularColor(1.f, 1.f, 1.f, 1.f);
            pMat->SetEmissiveColor({ c.x, c.y, c.z, 1.f });
            // Unique tag per colour. MeshRendererComponent::UpdateInstanceKey
            // hashes the material's tag; every clone here inherits the base
            // "Material" tag, so differently-coloured bullets hashed to the
            // same key and merged into one instancing bucket — where
            // TryRenderInstancedBucket binds only the first member's material,
            // painting the whole batch one colour (often white). Tagging by
            // colour keeps each colour in its own (correct) bucket. Mirrors
            // VoxelWorld's per-chunk mesh tag.
            pMat->SetTag("BulletMat_" + std::to_string(uColorRGB));
            m_pMeshRenderer->SetMaterial(pMat);
        }
    }

    void Bullet::OnBeginCollision(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float /*fDeltaTime*/)
    {
        if (!pDest) return;
        // Split child born inside its spawning enemy — ignore that enemy
        // entirely (no vanish here, no damage on the Enemy side) so the
        // Multiply hit doesn't immediately re-hit it. The child still
        // collides normally with every other enemy.
        if (IsIgnoring(pDest->GetGameObjectOwner())) return;

        // Weapon impact modules (Knockback / Gather / Burn / Slow) fire on every
        // enemy hit, before the OnHit despawn logic below — so a Vanish bullet
        // (which returns from the switch) still knocks back / gathers / burns.
        // Damage is applied separately by Enemy::OnCollision; these are the add-ons.
        if (!m_pImpactEffects.empty())
        {
            ImpactContext ctx;
            ctx.vImpactPos = m_pTransform ? m_pTransform->GetPosition() : Engine::Vector3();
            ctx.pHitEnemy  = pDest->GetGameObjectOwner();
            for (auto& pEffect : m_pImpactEffects)
                if (pEffect) pEffect->OnImpact(ctx);
        }

        // Bullets only respond to enemy bodies. The collider mask already
        // filters the pair list to ENEMY, but the tag check guards against
        // future additions sharing the BULLET/ENEMY pair (e.g. friendly
        // colliders).
        // No tag check — every callback delivered here is already filtered
        // to the ENEMY mask by Collider's group/mask system.

        // Max-hit cap: count this distinct enemy hit and, once the cap is
        // reached, despawn regardless of the on-hit persist behaviour — so a
        // NoChange/Reflect projectile pierces only that many enemies. 0 =
        // unlimited (the on-hit type decides). Field zones are duration-
        // based, so they don't tally hits here. The enemy still takes this
        // hit's damage via its own BEGIN callback.
        // Multiply is excluded — it splits + despawns on the first hit (no
        // piercing), so the cap never applies. Field is duration-based.
        if (m_eOnHit != OnHitEvent::Field && m_eOnHit != OnHitEvent::Multiply)
        {
            ++m_iHitCount;
            if (m_iMaxHits > 0 && m_iHitCount >= m_iMaxHits)
            {
                InActivate();
                return;
            }
        }

        switch (m_eOnHit)
        {
        case OnHitEvent::NoChange:
        case OnHitEvent::Field:
            // NoChange: orbital/piercing bullets stay alive. CollisionManager
            // won't re-fire BEGIN for the same collider until it leaves the
            // PrevColliderList, so we naturally damage each enemy once
            // per "engagement".
            // Field: the zone never despawns on contact — damage is applied
            // by Enemy on collision STAY (periodic tick), not here.
            break;

        case OnHitEvent::Reflect:
            // Flip forward direction by adding π to the yaw, and reset
            // the lifetime counter so the bullet survives long enough
            // to actually fly back.
            if (m_pTransform)
            {
                const float fRY = m_pTransform->GetRY();
                m_pTransform->SetRY(fRY + 3.14159265f);
            }
            m_fLifeAcc = 0.f;
            break;

        case OnHitEvent::Multiply:
            // Split + despawn on hit — no piercing. While split depth remains,
            // spawn the fanned children (each gets depth-1, so they split
            // again until depth hits 0, then just despawn). The parent is
            // always consumed by the hit.
            if (m_iSplitDepth > 0) SpawnSplitChildren(pDest->GetGameObjectOwner());
            InActivate();
            break;

        case OnHitEvent::Chain:
            // Remember the struck enemy (IsIgnoring now skips it) and redirect
            // at the nearest enemy not yet hit, then keep flying. The max-hit
            // cap above bounds the links; with nobody left to chain to, despawn.
            if (auto* pHit = pDest->GetGameObjectOwner()) m_vHitEnemies.push_back(pHit);
            if (m_pTransform)
            {
                const Engine::Vector3 vPos = m_pTransform->GetPosition();
                if (auto pNext = FindTargetEnemy(vPos, AimMode::Nearest, &m_vHitEnemies))
                {
                    if (auto pTr = pNext->GetComponent<Engine::Transform>())
                    {
                        const Engine::Vector3 t = pTr->GetPosition();
                        const float dx = t.x - vPos.x;
                        const float dz = t.z - vPos.z;
                        if (dx * dx + dz * dz > 1e-6f)
                            m_pTransform->SetRY(atan2f(-dx, -dz));
                    }
                    m_fLifeAcc = 0.f;   // reset lifetime so it reaches the next link
                }
                else
                {
                    InActivate();   // no fresh target left to chain to
                }
            }
            break;

        case OnHitEvent::Vanish:
        default:
            InActivate();
            break;
        }
    }

    void Bullet::SpawnSplitChildren(Engine::GameObject* pIgnore)
    {
        auto* pScene = GetScene();
        if (!pScene || !m_pTransform) return;
        const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iWeaponId);
        if (!pDef) return;

        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const float fYaw   = m_pTransform->GetRY();
        const Engine::Vector3 vPos = m_pTransform->GetPosition();
        // ±30° fan. Children stay Multiply but with one less split depth, so
        // they split again on their next hit until depth runs out (then they
        // just despawn). The decreasing depth bounds the 2^depth cascade.
        const float fSpread = 0.523f;   // ~30°
        for (int i = 0; i < 2; ++i)
        {
            auto pChild = pScene->CreateGameObject<Bullet>("bullet", pLayer);
            if (!pChild) continue;
            pChild->Configure(*pDef, m_iLevel, m_pOwner);
            // Configure reset depth to the weapon's full value — hand the
            // child one less than THIS bullet's remaining depth instead.
            pChild->m_iSplitDepth = m_iSplitDepth - 1;
            // Ignore the enemy we just hit so the child doesn't re-damage it
            // (or instantly re-split on it).
            pChild->m_pIgnoreTarget = pIgnore;
            if (auto pTr = pChild->GetTransform())
            {
                pTr->SetPosition(vPos);
                pTr->SetRX(-3.14159265f / 2.f);
                pTr->SetRY(fYaw + (i == 0 ? -fSpread : fSpread));
            }
        }
    }

    bool Bullet::TryVoxelReflect(float fDeltaTime)
    {
        // Only the Reflect on-hit bounces off walls; everything else keeps
        // phasing through voxels. Orbital (angular speed) and Fixed
        // (stationary) are excluded — wall reflection only makes sense for
        // forward-travelling projectiles.
        if (m_eOnHit != OnHitEvent::Reflect || !m_pVoxelWorld || !m_pTransform)
            return false;
        if (m_eMovement != MovementType::Straight && m_eMovement != MovementType::Spiral)
            return false;

        const float fStep = m_fSpeed * fDeltaTime;
        if (fStep <= 0.f) return false;

        // Forward heading in the XZ plane. The mesh is stood up with
        // SetRX(-PI/2), so model-forward maps to world (-sinθ, 0, -cosθ) at
        // yaw θ — same derivation as Player aim.
        const float fYaw = m_pTransform->GetRY();
        Engine::Vector3 vDir{ -sinf(fYaw), 0.f, -cosf(fYaw) };

        const Engine::Vector3 vPos = m_pTransform->GetPosition();
        // Cast one step + the bullet radius ahead so the bounce registers as
        // the wall surface is reached, not after the centre is embedded. The
        // ray stays at the bullet's Y (~kWallY+0.3), which sits in the wall
        // block layer, so it never mistakes the y=0 floor for a wall.
        const Engine::VoxelRaycastHit hit =
            m_pVoxelWorld->Raycast(vPos, vDir, fStep + m_fRadius);
        if (!hit.hit) return false;

        // Reflect across the struck face normal. The normal is an axis unit
        // vector, so reflection just negates that axis component.
        if (hit.faceX != 0) vDir.x = -vDir.x;
        if (hit.faceZ != 0) vDir.z = -vDir.z;
        // Origin already inside a solid cell (no boundary crossed → face is
        // all-zero): reverse fully so the bullet escapes instead of sticking.
        if (hit.faceX == 0 && hit.faceZ == 0) { vDir.x = -vDir.x; vDir.z = -vDir.z; }

        m_pTransform->SetRY(atan2f(-vDir.x, -vDir.z));
        return true;   // heading changed; skip this frame's forward step
    }

    void Bullet::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        if (!m_pTransform) return;

        // Apply per-weapon acceleration before handing m_fSpeed to the
        // strategy. Orbital reads m_fSpeed as angular rad/sec, so a
        // non-zero acceleration there is angular too — same field, same
        // unit convention as fProjectileSpeed.
        if (m_fAcceleration != 0.f)
            m_fSpeed += m_fAcceleration * fDeltaTime;

        // Reflect bullets bounce off voxel walls before the normal forward
        // step. A bounce reflects the heading and consumes this frame's
        // movement (the bullet advances next frame on the new heading), so
        // skip the strategy update when it fires.
        if (!TryVoxelReflect(fDeltaTime) && m_pMovement)
            m_pMovement->Update(*m_pTransform, m_fSpeed, fDeltaTime);

        // Tracer trail: commit a history point once the bullet has moved far
        // enough (so the ribbon follows curved/homing paths), cap the length,
        // and queue this frame's ribbon. The per-weapon style preset drives the
        // history density / length (and disables it entirely for None). m_fRadius
        // is the bullet's half-width, so the ribbon head matches the projectile.
        // Submitting here (before the despawn checks) keeps the streak visible
        // on the frame it dies.
        const TrailPreset& tp = TrailRenderManager::GetPreset(m_eTrailStyle);
        if (tp.iMaxPoints > 0)
        {
            const Engine::Vector3 vPos = m_pTransform->GetPosition();
            if (m_trail.empty() ||
                (vPos - m_trail.front()).LengthSq() >= tp.fMinDist * tp.fMinDist)
            {
                m_trail.push_front(vPos);
                if (static_cast<int>(m_trail.size()) > tp.iMaxPoints)
                    m_trail.pop_back();
            }
            if (m_trail.size() >= 2)
                TrailRenderManager::GetInst()->Submit(m_trail, m_vColor, m_fRadius, m_eTrailStyle);
        }

        // Let the movement strategy retire the bullet — a spiralling orbit
        // despawns once its radius passes kMaxOrbitRadius.
        if (m_pMovement && m_pMovement->WantsDespawn())
        {
            InActivate();
            return;
        }

        // Lifetime — Sustained weapons pass a huge lifetime so this
        // effectively never trips for orbital orbs.
        m_fLifeAcc += fDeltaTime;
        if (m_fLifeAcc >= m_fLifetime)
            InActivate();
    }
}

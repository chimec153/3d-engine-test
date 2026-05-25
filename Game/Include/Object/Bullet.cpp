#include "Bullet.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Bullet, Bullet)
#include "WeaponDatabase.h"
#include "Movement/BulletMovementFactory.h"
#include "Movement/IBulletMovement.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Collider.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/Texture.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Particle.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"

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
            // Hit radius is generous compared to the visual sphere
            // (~0.125 world units). A slow orbital orb glancing past
            // an enemy that's been blocked by the player's body needs
            // some margin to register the BEGIN — undersized collider
            // was the reason Orb felt like it phased through enemies.
            m_pCollider->SetRadius(0.28f);
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::BULLET);
            m_pCollider->SetMask(Engine::COLLISION_GROUP::ENEMY);
            // BEGIN callback — drives the OnHit enum (Vanish / Reflect /
            // Multiply / NoChange). Enemy::OnCollision still applies
            // damage but no longer InActivates the bullet.
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::BEGIN,
                this, &Bullet::OnBeginCollision);
        }

        m_pTrail = AddComponent<Engine::Particle>("trail", 64);
        if (m_pTrail)
        {
            std::shared_ptr<Engine::Texture> pTex =
                Engine::StaticCreateBindable<Engine::Texture>(
                    "particletexture", "/Game/Texture/Particle/particle_00.png", TEXTURE_PATH);
            m_pTrail->SetTexture(pTex);
            m_pTrail->SetStartColor({ 1.f, 0.85f, 0.2f, 1.0f });
            m_pTrail->SetEndColor  ({ 1.f, 0.20f, 0.0f, 0.0f });
            m_pTrail->SetStartSize ({ 0.20f, 0.20f });
            m_pTrail->SetEndSize   ({ 0.04f, 0.04f });
            m_pTrail->SetMaxLifeTime(0.5f);
            m_pTrail->SetEmitTime(0.01f);
            m_pTrail->SetAccelaration({ 0.f, 0.f, 0.f });
            m_pTrail->SetVelocity   ({ 0.f, -0.8f, 0.f });
            m_pTrail->SetMaxVelocity({ 0.f, -0.8f, 0.f });
            m_pTrail->SetMinCreatePosition({ -0.05f, -0.05f, -0.05f });
            m_pTrail->SetMaxCreatePosition({  0.05f,  0.05f,  0.05f });
            m_pTrail->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
        }

        return true;
    }

    void Bullet::Configure(const WeaponDef& def, int iLevel,
                           std::weak_ptr<Engine::Transform> pOwner)
    {
        m_eOnHit    = def.eOnHit;
        m_eFireMode = def.eFireMode;
        m_iWeaponId = def.iId;
        m_iLevel    = iLevel;
        m_pOwner    = pOwner;

        m_iDamage       = ComputeDamage(def, iLevel);
        m_fSpeed        = ComputeSpeed (def, iLevel);
        m_fAcceleration = def.fAcceleration;
        m_fLifetime     = def.fLifetime;
        m_fLifeAcc      = 0.f;

        ApplyShape(def.eShape, def.uColorRGB);

        // Per-weapon size — drives both the visual scale and the
        // collider radius (kept proportional to the legacy 0.28/0.25
        // ratio so a default-size bullet collides exactly as before).
        // ComputeSize folds in the per-level bump when the weapon's
        // level_up_field is Size, otherwise it returns def.fSize unchanged.
        const float fSize = ComputeSize(def, iLevel);
        if (m_pTransform)
            m_pTransform->SetScale(fSize, fSize, fSize);
        if (m_pCollider)
        {
            constexpr float kColliderToVisual = 0.28f / 0.25f;
            m_pCollider->SetRadius(fSize * kColliderToVisual);
        }

        // Swap in the movement strategy. Orbital's OnAttached seeds the
        // starting angle from the bullet transform yaw, which Player sets
        // per-instance for orb spread; other types ignore it.
        m_pMovement = MakeBulletMovement(def.eMovement, pOwner);
        if (m_pMovement && m_pTransform)
            m_pMovement->OnAttached(*m_pTransform);

        // Tint the trail too so the streak matches the projectile colour.
        if (m_pTrail)
        {
            auto col = Bullet_detail::ToFloat4(def.uColorRGB);
            m_pTrail->SetStartColor({ col.x, col.y, col.z, 1.0f });
            m_pTrail->SetEndColor  ({ col.x, col.y, col.z, 0.0f });
        }
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
        // Bullets only respond to enemy bodies. The collider mask already
        // filters the pair list to ENEMY, but the tag check guards against
        // future additions sharing the BULLET/ENEMY pair (e.g. friendly
        // colliders).
        // No tag check — every callback delivered here is already filtered
        // to the ENEMY mask by Collider's group/mask system.

        switch (m_eOnHit)
        {
        case OnHitEvent::NoChange:
            // Orbital/piercing bullets stay alive. CollisionManager won't
            // re-fire BEGIN for the same collider until it leaves the
            // PrevColliderList, so we naturally damage each enemy once
            // per "engagement".
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
            // Spawn two fanned children, then despawn ourselves so the
            // count doesn't keep doubling on subsequent enemies. The
            // just-hit enemy is passed through so the children ignore it.
            if (!m_bIsChild) SpawnSplitChildren(pDest->GetGameObjectOwner());
            InActivate();
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
        // ±30° fan. Children inherit weapon id / level but the m_bIsChild
        // flag stops them from multiplying again — otherwise a single hit
        // chain-reacts into hundreds of bullets.
        const float fSpread = 0.523f;   // ~30°
        for (int i = 0; i < 2; ++i)
        {
            auto pChild = pScene->CreateGameObject<Bullet>("bullet", pLayer);
            if (!pChild) continue;
            pChild->Configure(*pDef, m_iLevel, m_pOwner);
            pChild->m_bIsChild = true;
            // After Configure, override on-hit so the child can't split.
            pChild->m_eOnHit = OnHitEvent::Vanish;
            // Ignore the enemy we just hit so the child doesn't re-damage it.
            pChild->m_pIgnoreTarget = pIgnore;
            if (auto pTr = pChild->GetTransform())
            {
                pTr->SetPosition(vPos);
                pTr->SetRX(-3.14159265f / 2.f);
                pTr->SetRY(fYaw + (i == 0 ? -fSpread : fSpread));
            }
        }
    }

    void Bullet::Update(float fDeltaTime)
    {
        // Keep the trail anchored to the bullet's current pose so emitted
        // particles spawn where it actually is. Mirrors the original
        // pre-__super::Update sync from the legacy Bullet.
        if (m_pTrail && m_pTransform)
        {
            auto pTrailTr = m_pTrail->GetTransform();
            if (pTrailTr)
            {
                pTrailTr->SetPosition(m_pTransform->GetPosition());
                pTrailTr->SetRotation(m_pTransform->GetRotation());
            }
        }

        __super::Update(fDeltaTime);

        if (!m_pTransform) return;

        // Apply per-weapon acceleration before handing m_fSpeed to the
        // strategy. Orbital reads m_fSpeed as angular rad/sec, so a
        // non-zero acceleration there is angular too — same field, same
        // unit convention as fProjectileSpeed.
        if (m_fAcceleration != 0.f)
            m_fSpeed += m_fAcceleration * fDeltaTime;

        if (m_pMovement)
            m_pMovement->Update(*m_pTransform, m_fSpeed, fDeltaTime);

        // Lifetime — Sustained weapons pass a huge lifetime so this
        // effectively never trips for orbital orbs.
        m_fLifeAcc += fDeltaTime;
        if (m_fLifeAcc >= m_fLifetime)
            InActivate();
    }
}

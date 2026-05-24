#include "Bullet.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Bullet, Bullet)
#include "WeaponDatabase.h"
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
        m_eMovement(MovementType::Straight)
        , m_eOnHit(OnHitEvent::Vanish)
        , m_eFireMode(FireMode::Cooldown)
        , m_iDamage(5)
        , m_fSpeed(8.f)
        , m_fLifetime(2.f)
    {
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
        m_eMovement = def.eMovement;
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
        if (m_pTransform)
            m_pTransform->SetScale(def.fSize, def.fSize, def.fSize);
        if (m_pCollider)
        {
            constexpr float kColliderToVisual = 0.28f / 0.25f;
            m_pCollider->SetRadius(def.fSize * kColliderToVisual);
        }

        // Orbital bullets seed their starting angle from the player's yaw
        // so multiple orbs spread around the player instead of stacking.
        // Player::SpawnWeapon picks a phase offset for each instance via
        // an extra rotation around Y; the Y rotation passed in becomes
        // the initial orbit angle here. Phase 0 sits along world -Z (the
        // default forward direction).
        if (m_eMovement == MovementType::Orbital && m_pTransform)
            m_fOrbitAngle = m_pTransform->GetRY();

        // Tint the trail too so the streak matches the projectile colour.
        if (m_pTrail)
        {
            auto col = Bullet_detail::ToFloat4(def.uColorRGB);
            m_pTrail->SetStartColor({ col.x, col.y, col.z, 1.0f });
            m_pTrail->SetEndColor  ({ col.x, col.y, col.z, 0.0f });
        }
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
            // count doesn't keep doubling on subsequent enemies.
            if (!m_bIsChild) SpawnSplitChildren();
            InActivate();
            break;

        case OnHitEvent::Vanish:
        default:
            InActivate();
            break;
        }
    }

    void Bullet::SpawnSplitChildren()
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

        // Apply per-weapon acceleration before consuming m_fSpeed in the
        // switch below. Orbital reads m_fSpeed as angular rad/sec, so a
        // non-zero acceleration there is angular too — same field, same
        // unit convention as fProjectileSpeed.
        if (m_fAcceleration != 0.f)
            m_fSpeed += m_fAcceleration * fDeltaTime;

        switch (m_eMovement)
        {
        case MovementType::Straight:
        {
            // Local +Y is world-forward at the configured RX=-π/2 + RY=yaw.
            float fDist = fDeltaTime * m_fSpeed;
            m_pTransform->AddPosition(
                m_pTransform->GetAxis(Engine::AXIS_TYPE::Y) * fDist);
            break;
        }
        case MovementType::Spiral:
        {
            m_fSpiralTime += fDeltaTime;
            const float fDist = fDeltaTime * m_fSpeed;
            m_pTransform->AddPosition(
                m_pTransform->GetAxis(Engine::AXIS_TYPE::Y) * fDist);
            // Perpendicular sway: right axis is the bullet's local X
            // (after the RX=-π/2 + RY chain, local X is world-right).
            const float fAmp = 1.5f;
            const float fFreq = 6.f;
            const float fSide = std::cos(m_fSpiralTime * fFreq) * fAmp * fDeltaTime;
            m_pTransform->AddPosition(
                m_pTransform->GetAxis(Engine::AXIS_TYPE::X) * fSide);
            break;
        }
        case MovementType::Fixed:
            // Sits still until lifetime expires (e.g. mines, cursor-shots).
            break;
        case MovementType::Orbital:
        {
            // Sweep angle, then snap position relative to the owner.
            // m_fSpeed for Orbital encodes angular speed in rad/sec.
            m_fOrbitAngle += fDeltaTime * m_fSpeed;
            auto pOwner = m_pOwner.lock();
            if (pOwner)
            {
                const Engine::Vector3 vCenter = pOwner->GetPosition();
                const float ox = std::cos(m_fOrbitAngle) * m_fOrbitRadius;
                const float oz = std::sin(m_fOrbitAngle) * m_fOrbitRadius;
                // vCenter.y is the player's pivot (kWallY+1), but enemy
                // colliders sit around kWallY+0.3 — apply the muzzle-Y
                // offset Player pushed in via SetOrbitYOffset so the orb
                // crosses enemy bodies instead of orbiting above them.
                m_pTransform->SetPosition(
                    vCenter.x + ox,
                    vCenter.y + m_fOrbitYOffset,
                    vCenter.z + oz);
                // Face outward so spiral/triangle visuals point along
                // the tangent. Tangent yaw at angle θ is θ + π/2 in
                // world space; combine with the engine's forward
                // convention forward=(-sin, 0, -cos).
                m_pTransform->SetRY(-m_fOrbitAngle - 1.5707963f);
            }
            break;
        }
        default:
            break;
        }

        // Lifetime — Sustained weapons pass a huge lifetime so this
        // effectively never trips for orbital orbs.
        m_fLifeAcc += fDeltaTime;
        if (m_fLifeAcc >= m_fLifetime)
            InActivate();
    }
}

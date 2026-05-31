#include "Tower.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Tower, Tower)
#include "TowerManager.h"
#include "Attackable.h"
#include "AggroTarget.h"
#include "Bullet.h"
#include "Beam.h"
#include "Vfx/FragmentShatterManager.h"
#include "WeaponData.h"
#include "WeaponDatabase.h"
#include "GameStateManager.h"
#include "../GameDefs.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Camera.h"
#include "Component/MeshRendererComponent.h"
#include "Bindable/ColliderSphere.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "UI/Gauge.h"
#include <cmath>
#include <cfloat>

namespace Client
{
    Tower::Tower() = default;
    Tower::~Tower() = default;

    bool Tower::Init()
    {
        if (!__super::Init())
            return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Elongated cube: same x/z footprint as an enemy (~0.6 cell span)
        // but taller (1.6 high) so it reads as a turret, not an enemy block.
        // Base at local y=0 so SetCell can drop it straight on the floor top.
        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(Engine::MeshPresets::AxisBox(
                Engine::Vector3(-0.3f, 0.0f, -0.3f),
                Engine::Vector3( 0.3f, 1.6f,  0.3f)));
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

            // Clone the base material (same pattern as Bullet::ApplyShape) so
            // STANDARD_SOLID_PS reads a blue diffuse from the material cbuffer.
            // Unique tag keeps every tower in one (correct) instancing bucket.
            if (auto pSrcMat = Engine::StaticFindBindable<Engine::Material>("Material"))
            {
                m_pMaterial = std::static_pointer_cast<Engine::Material>(pSrcMat->Clone());
                m_pMaterial->SetDiffuseColor(0.2f, 0.45f, 0.95f, 1.f);
                m_pMaterial->SetEmissiveColor({ 0.05f, 0.10f, 0.25f, 1.f });
                m_pMaterial->SetTag("TowerMat");
                m_pMeshRenderer->SetMaterial(m_pMaterial);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
            }
        }

        // Seed this tower's weapon from the current placement default. The
        // shop can reassign it per-tower afterward (SetWeaponId).
        m_iWeaponId = TowerManager::GetInst().CurrentWeaponId();

        // Health (enemies melee this down) — no melee of its own, no blood /
        // paper-burn siblings, but it DOES get the impact-flash burst (last
        // arg true) so a hit reads visually. AggroTarget makes enemies prefer
        // towers over the player so they form the front line.
        m_pAttackable = AddComponent<Attackable>("tower_hp", kTowerHP, 0, 0, false, false, true);
        AddComponent<AggroTarget>("aggro", kTowerAggro);

        // Body collider so enemy projectiles can shoot the tower down. Own
        // TOWER group masking only BULLET (EnemyBullet's mask now includes
        // TOWER); enemy melee damages the tower directly (flow field), so this
        // only catches projectiles and never interferes with melee or pickups.
        // Pivot sits on the floor top (y=kWallY); EnemyBullet flies at the
        // enemy-collider height (kWallY+0.3), so offset the sphere up to that
        // lane and size it to cover the cube's x/z footprint corners (0.3√2).
        m_pCollider = AddComponent<Engine::ColliderSphere>("tower_body");
        if (m_pCollider)
        {
            m_pCollider->SetRadius(0.45f);
            m_pCollider->SetOffset({ 0.f, 0.3f, 0.f });
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::TOWER);
            m_pCollider->SetMask (Engine::COLLISION_GROUP::BULLET);
            m_pCollider->SetCallBack(Engine::COLLISION_TYPE::BEGIN, this, &Tower::OnHitByBullet);
        }

        // World-anchored HP bar. Pixel rect is rewritten every Update from
        // the projected head position, so the (0,0,0,0) seed here just
        // hides it for the first frame before Update fires.
        m_pHpBar = AddComponent<Engine::Gauge>("hpbar");
        if (m_pHpBar)
        {
            // Dark grey track + bright red fill. AABBGGRR memory order — same
            // convention the GameScene HUD HP/XP gauges use.
            m_pHpBar->SetColors(0xFF202020, 0xFF2030E0);
            m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
        }

        return true;
    }

    void Tower::SetCell(int cx, int cz)
    {
        if (!m_pTransform) return;
        // Cell centre on x/z; base on the floor top (y = kWallY) so the cube
        // stands on the ground rather than floating.
        m_pTransform->SetPosition(
            static_cast<float>(cx) + 0.5f,
            static_cast<float>(kWallY),
            static_cast<float>(cz) + 0.5f);
    }

    void Tower::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        // Broken — enemies meleed the HP to 0. Remove the tower.
        if (m_pAttackable && m_pAttackable->GetHP() <= 0)
        {
            // Burst the body into fragment shards (same CPU-shatter path the
            // enemy death uses). The cube is 1.6 tall on a pivot at the floor
            // top, so its centre of mass sits +0.8 above the pivot; shards rest
            // just above the floor (pivot y). Shards inherit the tower material
            // (blue, fading to red with damage). fScale = 1 assumes
            // tower_fragment.mesh was baked at the tower's real size.
            if (m_pTransform)
            {
                const Engine::Vector3 vBase = m_pTransform->GetPosition();
                Engine::Vector3 vBody = vBase;
                vBody.y += 0.8f;
                FragmentShatterManager::GetInst()->SpawnShatter(
                    FragmentShatterManager::VARIANT::TOWER,
                    vBody, 0.5f, m_pMaterial, vBase.y + 0.1f);
            }
            // Give up the owned slot: a destroyed tower must be re-bought in the
            // shop before another can be placed (the placement controller gates
            // on live-count < TowersOwned, so without this the freed cell would
            // be re-placeable for free).
            TowerManager::GetInst().RemoveTower();
            // Drop the persistent instances this tower owns (orbiting
            // sustained bullets + laser beams) — they're scene-owned and held
            // only by weak_ptr here, so without this they outlive the tower.
            // Same teardown the weapon-change path runs.
            for (auto& wp : m_vecSustained)
                if (auto sp = wp.lock()) sp->InActivate();
            m_vecSustained.clear();
            for (auto& wp : m_vecBeams)
                if (auto sp = wp.lock()) sp->InActivate();
            m_vecBeams.clear();
            // Snap the HP bar off-screen before deactivating so it doesn't
            // linger as a stuck full-width strip on the last rendered frame.
            if (m_pHpBar) m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
            InActivate();
            return;
        }

        // Project the head-of-tower world position into screen pixels and
        // rebuild the bar's pixel rect each frame.
        //
        // Modal gate: Tower is spawned at play time, AFTER the scene-init
        // modals (LevelUpChoices, StartWeaponSelect, PauseMenu) have already
        // registered their UIRenderers. RenderManager draws m_UIList in
        // insertion order with no z-sort, so without this gate the late-
        // registered tower bar would draw ON TOP of any modal that comes up
        // (level-up cards, weapon-select panels). Collapse to zero rect
        // whenever a modal is active — the bar reappears the frame the
        // state returns to Playing.
        if (m_pHpBar && !GameStateManager::GetInst().IsPlaying())
        {
            m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
        }
        else if (m_pHpBar && m_pTransform && m_pAttackable)
        {
            auto pCamera = Engine::Graphics::GetInst()->GetCamera(Engine::CAMERA_TYPE::NORMAL);
            if (pCamera)
            {
                // Tower mesh top sits at pivot.y + 1.6; add a small headroom
                // so the bar floats just above the cube rather than clipping
                // through it at glancing camera angles.
                Engine::Vector3 vHead = m_pTransform->GetPosition();
                vHead.y += 1.9f;
                float fPxX = 0.f, fPxY = 0.f, fPxW = 0.f;
                if (pCamera->WorldToScreen(vHead, fPxX, fPxY, fPxW))
                {
                    constexpr float kBarW = 50.f;
                    constexpr float kBarH = 5.f;
                    m_pHpBar->SetRectPx(
                        fPxX - kBarW * 0.5f,
                        fPxY - kBarH * 0.5f,
                        kBarW, kBarH);
                    const float fMax = static_cast<float>(m_pAttackable->GetMaxHP());
                    m_pHpBar->SetRatio(fMax > 0.f
                        ? static_cast<float>(m_pAttackable->GetHP()) / fMax
                        : 0.f);
                }
                else
                {
                    // Behind camera — collapse so nothing draws.
                    m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
                }
            }
        }

        // Damage feedback — tint from blue (full) toward dark red as HP drops.
        if (m_pMaterial && m_pAttackable && m_pAttackable->GetMaxHP() > 0)
        {
            const float f = static_cast<float>(m_pAttackable->GetHP()) /
                            static_cast<float>(m_pAttackable->GetMaxHP());
            m_pMaterial->SetDiffuseColor(
                0.2f + (1.f - f) * 0.7f,   // r: 0.2 → 0.9
                0.45f * f,                 // g: 0.45 → 0
                0.95f * f,                 // b: 0.95 → 0
                1.f);
        }

        // Only fire while actually playing — a modal (level-up / intermission /
        // pause) freezes the timer (dt ~ 0) anyway, but gate explicitly so a
        // target search never runs against a frozen world.
        if (!GameStateManager::GetInst().IsPlaying())
            return;

        const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iWeaponId);
        if (!pDef) return;   // no weapon equipped / invalid id

        // Weapon changed -> drop any persistent instances (orbiting bullets or
        // laser beams) spawned for the old one.
        if (m_iSustainedForId != m_iWeaponId)
        {
            for (auto& wp : m_vecSustained)
                if (auto sp = wp.lock()) sp->InActivate();
            m_vecSustained.clear();
            for (auto& wp : m_vecBeams)
                if (auto sp = wp.lock()) sp->InActivate();
            m_vecBeams.clear();
            m_iSustainedForId = m_iWeaponId;
        }

        // Sustained weapons don't auto-fire (cooldown 0 floors to 0.05s and
        // would spew ~20 bullets/sec). Instead they keep persistent instances:
        // a Straight + Sustained weapon is a laser Beam (anchored line driven
        // each frame toward the nearest enemy); everything else is a ring of
        // bullets orbiting the tower. (Re)spawn whenever none survive.
        if (pDef->eFireMode == FireMode::Sustained)
        {
            if (pDef->eMovement == MovementType::Straight)
            {
                bool bAnyAlive = false;
                for (auto& wp : m_vecBeams)
                    if (!wp.expired()) { bAnyAlive = true; break; }
                if (!bAnyAlive) RespawnBeams(*pDef);
                DriveBeams(fDeltaTime);
            }
            else
            {
                bool bAnyAlive = false;
                for (auto& wp : m_vecSustained)
                    if (!wp.expired()) { bAnyAlive = true; break; }
                if (!bAnyAlive) RespawnSustained(*pDef);
            }
            return;
        }

        const float fCooldown = ComputeCooldown(*pDef, m_iLevel);
        m_fCooldownAcc += fDeltaTime;
        if (m_fCooldownAcc < fCooldown) return;

        Engine::Vector3 vTarget;
        if (!FindNearestEnemy(vTarget))
        {
            // No target — don't bank cooldown indefinitely, just keep it ready
            // so the first in-range enemy is hit promptly.
            m_fCooldownAcc = fCooldown;
            return;
        }

        m_fCooldownAcc -= fCooldown;
        FireAt(vTarget, *pDef);
    }

    void Tower::OnHitByBullet(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float /*fDeltaTime*/)
    {
        // Only enemy projectiles. Mirror the enemy_bullet branch of
        // Player::CollisionPlayerBodyStay: pull the shooter's Attackable off
        // the bullet's owner, apply it to the tower's HP, then consume the
        // projectile. The tower self-destructs in Update when HP hits 0 (it
        // has no hit/die states to drive, unlike the player).
        if (pDest->GetTag() != "enemy_bullet") return;
        if (auto* pOwner = pDest->GetGameObjectOwner())
        {
            if (auto pAttacker = pOwner->GetComponent<Attackable>())
                pAttacker->Attack(m_pAttackable.get());
            pOwner->InActivate();
        }
    }

    bool Tower::FindNearestEnemy(Engine::Vector3& vOut) const
    {
        if (!m_pTransform) return false;
        auto pScene = GetScene();
        if (!pScene) return false;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return false;

        const Engine::Vector3 vPos = m_pTransform->GetPosition();
        const float fRangeSq = m_fRange * m_fRange;
        float fBestSq = FLT_MAX;
        bool  bFound = false;

        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;

            const Engine::Vector3 e = pTr->GetPosition();
            const float dx = e.x - vPos.x;
            const float dz = e.z - vPos.z;
            const float d2 = dx * dx + dz * dz;
            if (d2 > fRangeSq) continue;
            if (d2 < fBestSq) { fBestSq = d2; vOut = e; bFound = true; }
        }
        return bFound;
    }

    void Tower::FireAt(const Engine::Vector3& vTarget, const WeaponDef& def)
    {
        if (!m_pTransform) return;
        auto pScene = GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const Engine::Vector3 vTowerPos = m_pTransform->GetPosition();

        // Yaw from tower to target, same closed form Player uses
        // (forward = (-sinθ, 0, -cosθ) ⇒ θ = atan2(-dx, -dz)).
        const float dx = vTarget.x - vTowerPos.x;
        const float dz = vTarget.z - vTowerPos.z;
        const float fAimYaw = atan2f(-dx, -dz);
        const Engine::Vector3 vForward(-sinf(fAimYaw), 0.f, -cosf(fAimYaw));

        // Spawn at enemy-collider height (kWallY + 0.3) regardless of the
        // tower pivot, so bullets line up with enemy hitboxes — the same
        // altitude reasoning as Player's kMuzzleYOffset (see GameDefs.h).
        Engine::Vector3 vSpawn{
            vTowerPos.x,
            static_cast<float>(kWallY) + 0.3f,
            vTowerPos.z };
        // Front-origin weapons get the small muzzle push the player uses;
        // Mouse-origin has no cursor here, so treat it like Front.
        if (def.eOrigin == SpawnOrigin::Front || def.eOrigin == SpawnOrigin::Mouse)
            vSpawn = vSpawn + vForward * 0.6f;

        const int iCount = ComputeCount(def, m_iLevel);
        const float fFanStep = 0.174f;   // ~10°, matches Player's fan
        const float fFanBase = -fFanStep * (iCount - 1) * 0.5f;
        for (int i = 0; i < iCount; ++i)
        {
            auto pBullet = pScene->CreateGameObject<Bullet>("bullet", pLayer);
            if (!pBullet) continue;
            pBullet->Configure(def, m_iLevel, m_pTransform);
            pBullet->SetVoxelWorld(m_pVoxelWorld);
            // Orbital weapons re-anchor to the owner pivot each frame. The
            // tower pivot sits at y = kWallY, so lift the orbit +0.3 to circle
            // at enemy-collider height (kWallY + 0.3), matching the spawn Y.
            pBullet->SetOrbitYOffset(0.3f);
            if (auto pBulletTr = pBullet->GetTransform())
            {
                pBulletTr->SetPosition(vSpawn);
                pBulletTr->SetRX(-PI / 2.f);
                pBulletTr->SetRY(fAimYaw + fFanBase + fFanStep * i);
            }
        }
    }

    void Tower::RespawnSustained(const WeaponDef& def)
    {
        // Drop any survivors first (weapon swap / defensive re-spawn).
        for (auto& wp : m_vecSustained)
            if (auto sp = wp.lock()) sp->InActivate();
        m_vecSustained.clear();
        if (!m_pTransform) return;

        auto pScene = GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const int iCount = ComputeCount(def, m_iLevel);
        // Spawn at enemy-collider height (kWallY + 0.3), the same altitude
        // FireAt uses, so orbiting blades cross enemy hitboxes.
        Engine::Vector3 vSpawn = m_pTransform->GetPosition();
        vSpawn.y = static_cast<float>(kWallY) + 0.3f;
        for (int i = 0; i < iCount; ++i)
        {
            auto pBullet = pScene->CreateGameObject<Bullet>("bullet", pLayer);
            if (!pBullet) continue;
            if (auto pBulletTr = pBullet->GetTransform())
            {
                pBulletTr->SetPosition(vSpawn);
                pBulletTr->SetRX(-PI / 2.f);
                pBulletTr->SetRY((6.2831853f * i) / iCount);   // even ring
            }
            // Owner = this tower, so OrbitalMovement circles the tower rather
            // than the player. Non-orbital Sustained types just fly/sit per
            // their movement from the tower (Fixed = aura at the tower).
            pBullet->Configure(def, m_iLevel, m_pTransform);
            pBullet->SetVoxelWorld(m_pVoxelWorld);
            pBullet->SetOrbitYOffset(0.3f);
            m_vecSustained.emplace_back(pBullet);
        }
    }

    void Tower::RespawnBeams(const WeaponDef& def)
    {
        // Drop any survivors first (weapon swap / defensive re-spawn).
        for (auto& wp : m_vecBeams)
            if (auto sp = wp.lock()) sp->InActivate();
        m_vecBeams.clear();
        if (!m_pTransform) return;

        auto pScene = GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        // One persistent Beam per count (mirrors Player::RespawnBeams). They
        // all aim the same way from the tower, but DriveBeams reads the live
        // weapon level each frame, so a level-up takes effect without respawn.
        const int iCount = ComputeCount(def, m_iLevel);
        for (int i = 0; i < iCount; ++i)
        {
            auto pBeam = pScene->CreateGameObject<Beam>("beam", pLayer);
            if (!pBeam) continue;
            pBeam->Configure(m_iWeaponId, m_iLevel);
            // Lock the heading per on-pulse: the tower auto-aims, so without
            // this the beam would swivel to track the nearest enemy every
            // frame. It now fires a fixed straight shot and only re-acquires
            // between pulses (during the duty-cycle off phase).
            pBeam->SetAimLock(true);
            m_vecBeams.emplace_back(pBeam);
        }
    }

    void Tower::DriveBeams(float fDeltaTime)
    {
        if (!m_pTransform) return;

        // Re-aim at the nearest enemy when one is in range; otherwise keep the
        // last heading so the beam idles in place rather than snapping to 0.
        Engine::Vector3 vTarget;
        if (FindNearestEnemy(vTarget))
        {
            const Engine::Vector3 vPos = m_pTransform->GetPosition();
            const float dx = vTarget.x - vPos.x;
            const float dz = vTarget.z - vPos.z;
            m_fBeamYaw = atan2f(-dx, -dz);
        }

        // Anchor at enemy-collider height (kWallY + 0.3), same lane FireAt and
        // RespawnSustained use, so the beam crosses enemy hitboxes.
        Engine::Vector3 vAnchor = m_pTransform->GetPosition();
        vAnchor.y = static_cast<float>(kWallY) + 0.3f;
        for (auto& wp : m_vecBeams)
            if (auto sp = wp.lock())
                sp->Drive(vAnchor, m_fBeamYaw, fDeltaTime);
    }
}

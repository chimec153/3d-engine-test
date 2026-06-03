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
#include "TowerData.h"
#include "Impact/ImpactEffectFactory.h"
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
#include "../UI/DamageText.h"
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <cstdio>

namespace Client
{
    Tower::Tower() = default;
    Tower::~Tower() = default;

    void Tower::SetLevel(int iLevel)
    {
        if (iLevel < 1)                iLevel = 1;
        if (iLevel > kMaxWeaponLevel)  iLevel = kMaxWeaponLevel;
        if (iLevel == m_iLevel) return;
        m_iLevel = iLevel;
        // Sustained / Beam towers cache their instances; invalidating the
        // spawned-for id makes Update tear them down and respawn at the new
        // level's ComputeCount (cooldown towers pick up the level next shot).
        m_iSustainedForId = -1;
    }

    void Tower::SetTowerDefId(int iId)
    {
        m_iTowerDefId = iId;
        // -1 (or an unknown id) means the default attack type — exactly what
        // Init already seeded, so resolve to the same FirstOfKind(Attack).
        const TowerDef* pDef = (iId >= 0) ? TowerDatabase::GetInst().Get(iId) : nullptr;
        if (!pDef) pDef = TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
        if (!pDef) return;   // no table loaded — keep the Init defaults
        // Store the RESOLVED id so a -1 (default) tower and an explicitly-bought
        // first-attack tower count as the SAME type for merging.
        m_iTowerDefId = pDef->iId;

        // Use-time combat members (read on every shot) — safe to overwrite.
        m_fAttack        = pDef->fAttack;
        m_fAttackSpeed   = pDef->fAttackSpeed;
        m_fCritChance    = pDef->fCritChance;
        m_fCritMult      = pDef->fCritMult;
        m_fRange         = pDef->fRange;
        m_uTowerImpact   = pDef->uTowerImpact;
        m_fTowerEffectP0 = pDef->fTowerEffectP0;
        m_fTowerEffectP1 = pDef->fTowerEffectP1;

        // HP / defence are baked into the Attackable. Shift only the base
        // portion by the delta from what Init seeded so the level-up bonus
        // (TowerBonusHP/Def) layered on top is preserved.
        if (m_pAttackable)
        {
            const int   iHpDelta  = pDef->iHP - m_iSeedBaseHP;
            if (iHpDelta != 0)    m_pAttackable->AddMaxHP(iHpDelta);
            const float fDefDelta = pDef->fDefense - m_fSeedBaseDef;
            if (fDefDelta != 0.f) m_pAttackable->AddDamageReduction(fDefDelta);
        }
        m_iSeedBaseHP  = pDef->iHP;
        m_fSeedBaseDef = pDef->fDefense;

        // --- Per-type body: shape (N-gon prism) + identity colour ------------
        // Shape: the mesh key "prismN" picks an N-gon prism (side-count reads the
        // type at a glance); anything else falls back to a square prism.
        int iSides = 4;
        if (pDef->strMesh.rfind("prism", 0) == 0)   // starts with "prism"
        {
            const int n = std::atoi(pDef->strMesh.c_str() + 5);
            if (n >= 3) iSides = n;
        }
        if (m_pMeshRenderer)
        {
            // Same footprint/height envelope as the original box (radius ~0.32
            // ≈ the 0.3 half-extent; 0 → 1.6 tall). Cached per side-count, so
            // same-shape towers share one Mesh.
            m_pMeshRenderer->SetMesh(Engine::MeshPresets::RegularPrism(iSides, 0.32f, 0.f, 1.6f));
        }

        // Colour: one identity hue per kind (NOTE: a unique material tag was set
        // in Init, so this shows per-tower instead of collapsing to tower #0).
        switch (pDef->eKind)
        {
        case TowerKind::Frost:   m_vBaseColor = { 0.30f, 0.70f, 1.00f }; break;  // cyan
        case TowerKind::Mortar:  m_vBaseColor = { 1.00f, 0.50f, 0.15f }; break;  // orange
        case TowerKind::Gravity: m_vBaseColor = { 0.80f, 0.30f, 0.90f }; break;  // magenta
        case TowerKind::Buff:    m_vBaseColor = { 1.00f, 0.84f, 0.20f }; break;  // gold
        case TowerKind::Heal:    m_vBaseColor = { 0.30f, 0.90f, 0.50f }; break;  // green
        case TowerKind::Attack:
        default:                 m_vBaseColor = { 0.55f, 0.62f, 0.75f }; break;  // steel
        }
        if (m_pMaterial)
        {
            m_pMaterial->SetDiffuseColor(m_vBaseColor.x, m_vBaseColor.y, m_vBaseColor.z, 1.f);
            m_pMaterial->SetEmissiveColor({ m_vBaseColor.x * 0.25f, m_vBaseColor.y * 0.25f, m_vBaseColor.z * 0.25f, 1.f });
        }
        // NOTE: aggro stays at the Init default this step.
    }

    void Tower::Despawn()
    {
        // Same instance teardown as the HP-0 break branch in Update: the
        // orbiting sustained bullets + laser beams are scene-owned and held
        // here only by weak_ptr, so they'd outlive the tower without this.
        for (auto& wp : m_vecSustained)
            if (auto sp = wp.lock()) sp->InActivate();
        m_vecSustained.clear();
        for (auto& wp : m_vecBeams)
            if (auto sp = wp.lock()) sp->InActivate();
        m_vecBeams.clear();
        InActivate();
    }

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
                // UNIQUE per-tower tag: instancing buckets by mesh+material tag and
                // draws the whole bucket with the FIRST member's mesh/material. A
                // shared "TowerMat" tag therefore made every tower render as tower
                // #0 (same colour/shape, broken per-tower damage tint). A unique
                // tag makes each tower a solo draw so its type colour, prism mesh
                // and hit tint all show correctly (towers are few — cost is moot).
                static int s_iMatSeq = 0;
                char szMatTag[32];
                std::snprintf(szMatTag, sizeof(szMatTag), "TowerMat_%d", s_iMatSeq++);
                m_pMaterial->SetTag(szMatTag);
                m_pMeshRenderer->SetMaterial(m_pMaterial);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
            }
        }

        // Seed this tower's weapon from the current placement default. The
        // shop can reassign it per-tower afterward (SetWeaponId).
        m_iWeaponId = TowerManager::GetInst().CurrentWeaponId();

        // Base stats from towers.csv (falls back to the GameDefs constants when
        // no row is loaded). These drive HP / defence / range / aggro below and
        // the weapon-scaling combat stats used in the fire path.
        const TowerDef* pTowerDef = TowerDatabase::GetInst().FirstOfKind(TowerKind::Attack);
        const int   iBaseHP    = pTowerDef ? pTowerDef->iHP    : kTowerHP;
        const float fBaseDef   = pTowerDef ? pTowerDef->fDefense : 0.f;
        const int   iAggro     = pTowerDef ? pTowerDef->iAggro : kTowerAggro;
        // Record what we seed from the default type so SetTowerDefId can later
        // shift HP/defence by only the chosen type's delta.
        m_iSeedBaseHP  = iBaseHP;
        m_fSeedBaseDef = fBaseDef;
        if (pTowerDef)
        {
            m_fAttack      = pTowerDef->fAttack;
            m_fAttackSpeed = pTowerDef->fAttackSpeed;
            m_fCritChance  = pTowerDef->fCritChance;
            m_fCritMult    = pTowerDef->fCritMult;
            m_fRange       = pTowerDef->fRange;
            // Intrinsic on-hit effect this tower layers onto its weapon's bullets.
            m_uTowerImpact   = pTowerDef->uTowerImpact;
            m_fTowerEffectP0 = pTowerDef->fTowerEffectP0;
            m_fTowerEffectP1 = pTowerDef->fTowerEffectP1;
        }

        // Health (enemies melee this down) — no melee of its own, no blood /
        // paper-burn siblings, but it DOES get the impact-flash burst (last
        // arg true) so a hit reads visually. AggroTarget makes enemies prefer
        // towers over the player so they form the front line.
        m_pAttackable = AddComponent<Attackable>("tower_hp", iBaseHP, 0, 0, false, false, true);
        // Apply the tower's base defence, then the cumulative level-up tower HP
        // / defence buffs so a tower placed after those upgrades starts stronger
        // (already-placed towers were bumped when the card was picked — see
        // Player::ApplyStatUpgrade).
        if (m_pAttackable)
        {
            if (fBaseDef != 0.f) m_pAttackable->AddDamageReduction(fBaseDef);
            const int   iBonusHP = TowerManager::GetInst().TowerBonusHP();
            const float fBonusDef = TowerManager::GetInst().TowerBonusDef();
            if (iBonusHP  != 0)   m_pAttackable->AddMaxHP(iBonusHP);
            if (fBonusDef != 0.f) m_pAttackable->AddDamageReduction(fBonusDef);
        }
        AddComponent<AggroTarget>("aggro", iAggro);

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

    void Tower::OnHealed(int iAmount)
    {
        if (iAmount <= 0) return;
        // Green body pulse via the per-actor hit-flash channel (every PS
        // variant lerps value0 toward this colour by intensity); Update decays
        // it. Same channel the enemies use for the red hit flash.
        if (m_pMaterial)
            m_pMaterial->SetHitFlash(Engine::Vector3(0.2f, 1.f, 0.45f), 1.f);
        // Green "+N" number above the head (same offset as the HP bar).
        if (m_pTransform)
        {
            Engine::Vector3 vHead = m_pTransform->GetPosition();
            vHead.y += 1.9f;
            DamageTextManager::GetInst()->Spawn(
                vHead, iAmount, false, reinterpret_cast<uintptr_t>(this), true);
        }
    }

    void Tower::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        // Death squish: flatten + widen the cube over kSquishTime, then burst it
        // into shards and remove. The slot release / instance teardown already
        // ran when it broke (below), so this only animates the body and pops it.
        if (m_bSquishing)
        {
            m_fSquish += fDeltaTime;
            float t = m_fSquish / kSquishTime;
            if (t > 1.f) t = 1.f;
            const float ease = 1.f - (1.f - t) * (1.f - t);
            const float sy  = 1.f + (kSquishFlatY  - 1.f) * ease;
            const float sxz = 1.f + (kSquishWideXZ - 1.f) * ease;
            if (m_pTransform) m_pTransform->SetScale(sxz, sy, sxz);
            if (m_fSquish >= kSquishTime)
            {
                // Burst the body into fragment shards (same CPU-shatter path the
                // enemy death uses). The cube is 1.6 tall on a pivot at the floor
                // top, so its centre of mass sits +0.8 above the pivot; shards
                // rest just above the floor (pivot y). Shards inherit the tower
                // material (blue, fading to red with damage). fScale = 1 assumes
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
                InActivate();
            }
            return;
        }

        // Broken — enemies meleed the HP to 0. Free the slot + tear down owned
        // instances now, then start the death squish (shatter fires at its end).
        if (m_pAttackable && m_pAttackable->GetHP() <= 0)
        {
            // Bench this tower until the next round: ownership is kept (not
            // re-bought), but it returns to the reserve carrying its weapon +
            // level, flagged on destroy-cooldown so it can't be re-placed this
            // round (OnNewRound clears the flag at the next round start).
            TowerManager::GetInst().DestroyTower(m_iWeaponId, m_iLevel, m_iTowerDefId);
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
            // Snap the HP bar off-screen so it doesn't linger as a stuck
            // full-width strip while the body squishes.
            if (m_pHpBar) m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
            m_bSquishing = true;
            m_fSquish    = 0.f;
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

        // Damage feedback — tint from the type's identity colour (full HP)
        // toward red as HP drops.
        if (m_pMaterial && m_pAttackable && m_pAttackable->GetMaxHP() > 0)
        {
            const float f = static_cast<float>(m_pAttackable->GetHP()) /
                            static_cast<float>(m_pAttackable->GetMaxHP());
            m_pMaterial->SetDiffuseColor(
                m_vBaseColor.x + (1.f - f) * (1.f - m_vBaseColor.x),   // → red 1.0
                m_vBaseColor.y * f,                                    // → 0
                m_vBaseColor.z * f,                                    // → 0
                1.f);
        }
        // Decay the green heal-flash pulse (set by OnHealed). Composes with the
        // HP tint above — different channel (PS lerps value0 toward the flash
        // colour by intensity). No-op once the pulse has decayed to 0.
        if (m_pMaterial) m_pMaterial->TickHitFlash(fDeltaTime, 4.f);

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

        // Fire rate = the tower's base attack speed (towers.csv) times the
        // level-up fire-rate buff; both shorten the cooldown (mult > 1 = faster).
        float fCooldown = ComputeCooldown(*pDef, m_iLevel);
        const float fRateMult = TowerManager::GetInst().TowerFireRateMult() * m_fAttackSpeed;
        if (fRateMult > 0.f) fCooldown /= fRateMult;
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
            // Damage scaling, player-style: tower base attack x the level-up
            // tower-atk buff, then a per-shot crit roll multiplies by crit_mult.
            // (crit uses std::rand like Player — <random> is banned here, see
            // the epsilon-macro note in the codebase memory.)
            {
                float fScale = m_fAttack * TowerManager::GetInst().TowerAtkMult();
                if (m_fCritChance > 0.f &&
                    (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) < m_fCritChance)
                    fScale *= m_fCritMult;
                pBullet->ScaleDamage(fScale);
            }
            pBullet->SetVoxelWorld(m_pVoxelWorld);
            // Orbital weapons re-anchor to the owner pivot each frame. The
            // tower pivot sits at y = kWallY, so lift the orbit +0.3 to circle
            // at enemy-collider height (kWallY + 0.3), matching the spawn Y.
            pBullet->SetOrbitYOffset(0.3f);
            // Layer the tower's intrinsic effect on top of the weapon's effects.
            ApplyTowerImpact(pBullet.get());
            if (auto pBulletTr = pBullet->GetTransform())
            {
                pBulletTr->SetPosition(vSpawn);
                pBulletTr->SetRX(-PI / 2.f);
                pBulletTr->SetRY(fAimYaw + fFanBase + fFanStep * i);
            }
        }
    }

    void Tower::ApplyTowerImpact(Bullet* pBullet)
    {
        if (!pBullet || m_uTowerImpact == Impact_None) return;
        for (auto& pEffect : MakeTowerImpactEffects(m_uTowerImpact, m_fTowerEffectP0, m_fTowerEffectP1))
            pBullet->AddImpactEffect(std::move(pEffect));
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
            // Base attack x level-up buff (no per-shot crit roll on persistent
            // orbiting instances — crit is a per-fire event).
            pBullet->ScaleDamage(m_fAttack * TowerManager::GetInst().TowerAtkMult());
            pBullet->SetVoxelWorld(m_pVoxelWorld);
            pBullet->SetOrbitYOffset(0.3f);
            // Layer the tower's intrinsic effect on top of the weapon's effects.
            ApplyTowerImpact(pBullet.get());
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

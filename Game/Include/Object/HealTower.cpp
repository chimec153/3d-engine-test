#include "HealTower.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::HealTower, HealTower)
#include "Attackable.h"
#include "AggroTarget.h"
#include "Vfx/FragmentShatterManager.h"
#include "TowerManager.h"
#include "TowerData.h"
#include "GameStateManager.h"
#include "../GameDefs.h"
#include "Bindable/Decal.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Camera.h"
#include "Component/MeshRendererComponent.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "UI/Gauge.h"
#include "Types.h"
#include <cmath>
#include <vector>

namespace Client
{
    std::shared_ptr<Engine::Mesh> HealTower::BuildCylinderMesh()
    {
        const char* pTag = "MeshPreset.HealCylinder";
        if (auto p = Engine::StaticFindBindable<Engine::Mesh>(pTag)) return p;

        // Y-axis cylinder, base at y=0. Engine convention (see
        // MeshPresets::AxisBox) is CW-front with the geometric normal —
        // (b-a)×(c-a) — pointing outward. The vertex sequences below list
        // verts in CCW order, so addTri swaps b/c on emit to flip winding
        // outward in one place instead of touching every call site.
        const int   N = 18;
        const float r = 0.35f;
        const float h = 1.4f;
        std::vector<Engine::VertexStandard> v;
        std::vector<unsigned int>           idx;

        auto vert = [](float px, float py, float pz, float nx, float ny, float nz)
        {
            Engine::VertexStandard s;
            s.pos = { px, py, pz };
            s.normal = { nx, ny, nz };
            return s;
        };
        auto addTri = [&](const Engine::VertexStandard& a,
                          const Engine::VertexStandard& b,
                          const Engine::VertexStandard& c)
        {
            const unsigned int base = static_cast<unsigned int>(v.size());
            v.push_back(a); v.push_back(b); v.push_back(c);
            idx.push_back(base); idx.push_back(base + 2); idx.push_back(base + 1);
        };

        for (int s = 0; s < N; ++s)
        {
            const float a0 = static_cast<float>(s)     / N * 2.f * PI;
            const float a1 = static_cast<float>(s + 1) / N * 2.f * PI;
            const float c0 = std::cos(a0), z0 = std::sin(a0);
            const float c1 = std::cos(a1), z1 = std::sin(a1);

            // Side (radial normals).
            addTri(vert(r * c0, 0.f, r * z0, c0, 0.f, z0),
                   vert(r * c1, 0.f, r * z1, c1, 0.f, z1),
                   vert(r * c1, h,   r * z1, c1, 0.f, z1));
            addTri(vert(r * c0, 0.f, r * z0, c0, 0.f, z0),
                   vert(r * c1, h,   r * z1, c1, 0.f, z1),
                   vert(r * c0, h,   r * z0, c0, 0.f, z0));
            // Top cap.
            addTri(vert(0.f, h, 0.f, 0.f, 1.f, 0.f),
                   vert(r * c0, h, r * z0, 0.f, 1.f, 0.f),
                   vert(r * c1, h, r * z1, 0.f, 1.f, 0.f));
            // Bottom cap.
            addTri(vert(0.f, 0.f, 0.f, 0.f, -1.f, 0.f),
                   vert(r * c1, 0.f, r * z1, 0.f, -1.f, 0.f),
                   vert(r * c0, 0.f, r * z0, 0.f, -1.f, 0.f));
        }

        return Engine::StaticCreateBindable<Engine::Mesh>(pTag, v, idx);
    }

    HealTower::HealTower()  = default;
    HealTower::~HealTower() = default;

    bool HealTower::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(BuildCylinderMesh());
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

            if (auto pSrcMat = Engine::StaticFindBindable<Engine::Material>("Material"))
            {
                m_pMaterial = std::static_pointer_cast<Engine::Material>(pSrcMat->Clone());
                m_pMaterial->SetDiffuseColor(0.2f, 0.9f, 0.4f, 1.f);   // heal green
                m_pMaterial->SetEmissiveColor({ 0.05f, 0.25f, 0.10f, 1.f });
                m_pMaterial->SetTag("HealTowerMat");
                m_pMeshRenderer->SetMaterial(m_pMaterial);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
            }
        }

        // Base stats from towers.csv (falls back to the GameDefs constants when
        // no heal row is loaded). Heal amount / interval / radius are cached on
        // members; HP / defence / aggro drive the tower object.
        const TowerDef* pDef = TowerDatabase::GetInst().FirstOfKind(TowerKind::Heal);
        const int   iBaseHP  = pDef ? pDef->iHP    : kHealTowerHP;
        const float fBaseDef = pDef ? pDef->fDefense : 0.f;
        const int   iAggro   = pDef ? pDef->iAggro : kTowerAggro;
        m_iHealAmount   = pDef ? pDef->iHealAmount   : kHealAmount;
        m_fHealInterval = pDef ? pDef->fHealInterval : kHealInterval;
        m_fHealRadius   = pDef ? pDef->fHealRadius   : kHealRadius;

        // HP + aggro so enemies attack it like any tower; breaks at 0.
        // Impact-flash burst on hit (last arg true), no blood/paper-burn.
        m_pAttackable = AddComponent<Attackable>("tower_hp", iBaseHP, 0, 0, false, false, true);
        if (m_pAttackable && fBaseDef != 0.f) m_pAttackable->AddDamageReduction(fBaseDef);
        AddComponent<AggroTarget>("aggro", iAggro);

        // World-anchored HP bar — see Tower::Init for the contract. Seed is
        // a zero rect so we don't flash a full-width bar before the first
        // Update projects the head position.
        m_pHpBar = AddComponent<Engine::Gauge>("hpbar");
        if (m_pHpBar)
        {
            m_pHpBar->SetColors(0xFF202020, 0xFF2030E0);
            m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
        }

        // Heal telegraph floor decals: a static outer ring + an inner disc that
        // grows from the centre (Update scales it by charge progress). Both use
        // procedural shaders (no texture); the material green tints the writer's
        // diffuse output. Decal default render layer is DECAL.
        auto makeDecal = [&](const char* tag) -> std::shared_ptr<Engine::Decal>
        {
            auto pDecal = AddComponent<Engine::Decal>(tag);
            if (!pDecal) return nullptr;
            pDecal->SetMesh(Engine::MeshPresets::AxisBox(
                Engine::Vector3(-0.5f, -0.5f, -0.5f),
                Engine::Vector3( 0.5f,  0.5f,  0.5f)));
            // Procedural floor decal: the ring/disc shape is computed in the
            // shader from the in-circle UV (no texture). Both writers emit a
            // clean up-normal + neutral material so the unbound normal/spec/
            // emissive slots don't light the floor decal black (PS_DECAL is the
            // full-material blood-decal shader).
            pDecal->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>(DECAL_PS_RING));
            pDecal->SetTopology(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            if (auto pSrc = Engine::StaticFindBindable<Engine::Material>("Material"))
            {
                auto pMat = std::static_pointer_cast<Engine::Material>(pSrc->Clone());
                pMat->SetDiffuseColor(0.2f, 1.0f, 0.4f, 1.f);     // heal green
                pMat->SetEmissiveColor({ 0.f, 0.f, 0.f, 0.f });   // unbound slot: kill garbage
                pMat->SetTag(tag);
                pDecal->SetMaterial(pMat);
            }
            pDecal->StartFade();
            pDecal->SetMaxFadeTime(m_fHealInterval);
            return pDecal;
        };
        m_pRingDecal = makeDecal("heal_ring_decal");

        return true;
    }

    void HealTower::SetCell(int cx, int cz)
    {
        if (!m_pTransform) return;
        m_pTransform->SetPosition(
            static_cast<float>(cx) + 0.5f,
            static_cast<float>(kWallY),
            static_cast<float>(cz) + 0.5f);
    }

    void HealTower::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        // Broken.
        if (m_pAttackable && m_pAttackable->GetHP() <= 0)
        {
            // Burst into fragment shards (heal_tower_fragment.mesh). The cylinder
            // is 1.4 tall on a pivot at the floor top → centre of mass +0.7;
            // shards rest just above the floor. Shards inherit the heal-green
            // material. fScale = 1 assumes the mesh was baked at real size.
            if (m_pTransform)
            {
                const Engine::Vector3 vBase = m_pTransform->GetPosition();
                Engine::Vector3 vBody = vBase;
                vBody.y += 0.7f;
                FragmentShatterManager::GetInst()->SpawnShatter(
                    FragmentShatterManager::VARIANT::HEAL_TOWER,
                    vBody, 0.5f, m_pMaterial, vBase.y + 0.1f);
            }
            // Give up the owned slot so a destroyed heal tower must be re-bought
            // before another can be placed (mirrors Tower).
            TowerManager::GetInst().RemoveHealTower();
            if (m_pHpBar) m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
            InActivate();
            return;
        }

        // HP bar — projected from the cylinder top each frame. Mirrors
        // Tower::Update; the only difference is the head-Y offset (the
        // cylinder is 1.4 tall vs. the cube's 1.6). Hides on any modal
        // state — same registration-order rationale as Tower (see Tower.cpp).
        if (m_pHpBar && !GameStateManager::GetInst().IsPlaying())
        {
            m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
        }
        else if (m_pHpBar && m_pTransform && m_pAttackable)
        {
            auto pCamera = Engine::Graphics::GetInst()->GetCamera(Engine::CAMERA_TYPE::NORMAL);
            if (pCamera)
            {
                Engine::Vector3 vHead = m_pTransform->GetPosition();
                vHead.y += 1.7f;   // cylinder top (pivot+1.4) + small headroom
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
                    m_pHpBar->SetRectPx(0.f, 0.f, 0.f, 0.f);
                }
            }
        }

        if (!GameStateManager::GetInst().IsPlaying())
            return;

        // Charge the heal pulse.
        m_fHealAcc += fDeltaTime;
        if (m_fHealInterval > 0.f && m_fHealAcc >= m_fHealInterval)
        {
            m_fHealAcc -= m_fHealInterval;
            HealNearbyAllies();
            m_pRingDecal->SetFadeTime(0.f);
        }

        // Telegraph decals: a fixed outer ring + an inner disc that grows from
        // the centre toward kHealRadius as the pulse charges (snaps back on
        // pulse). The decal box is centred on the tower's floor surface; XZ
        // scale = the circle diameter (footprint). Y scale (kBoxH) is TALL so
        // the box's screen silhouette covers the whole circle from any camera
        // angle (a thin box's coverage shrank/shifted with the camera). The
        // PS_DECAL_FLAT shader then clips to a thin floor band on localpos.y, so
        // the player/enemies standing in the circle are NOT painted.
        if (m_pTransform)
        {
            constexpr float kBoxH = 2.0f;
            const float fFill = m_fHealInterval > 0.f ? m_fHealAcc / m_fHealInterval : 0.f;   // 0..1
            const Engine::Vector3 vPos = m_pTransform->GetPosition();
            const Engine::Vector3 vCentre(vPos.x, static_cast<float>(kWallY), vPos.z);
            const float fRingD = m_fHealRadius * 2.f;

            if (m_pRingDecal)
            {
                if (auto pT = m_pRingDecal->GetTransform())
                {
                    pT->SetScale(fRingD, kBoxH, fRingD);
                    pT->SetPosition(vCentre);
                }
            }
        }
    }

    void HealTower::HealNearbyAllies()
    {
        auto pScene = GetScene();
        if (!pScene || !m_pTransform) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        const Engine::Vector3 vPos = m_pTransform->GetPosition();
        const float fR2 = m_fHealRadius * m_fHealRadius;

        for (const auto& p : pLayer->GetGameObjectList())
        {
            if (!p || !p->IsActive()) continue;
            // Allies = things enemies target (player + towers) — they carry an
            // AggroTarget. Enemies don't, so they're never healed.
            if (!p->GetComponent<AggroTarget>()) continue;
            auto pTr = p->GetComponent<Engine::Transform>();
            if (!pTr) continue;
            const Engine::Vector3 e = pTr->GetPosition();
            const float dx = e.x - vPos.x;
            const float dz = e.z - vPos.z;
            if (dx * dx + dz * dz > fR2) continue;
            if (auto pHP = p->GetComponent<Attackable>())
                pHP->Heal(m_iHealAmount);
        }
    }
}

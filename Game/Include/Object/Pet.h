#pragma once
#include "GameObject/GameObject.h"
#include "WeaponData.h"
#include "WeaponDatabase.h"
#include "Bullet.h"
#include "Vector3.h"
#include "Types.h"                        // PI, DEFAULT_LAYER
#include "Movement/EnemyTargeting.h"      // FindTargetEnemy
#include "Bindable/Transform.h"
#include "Bindable/Mesh.h"
#include "Bindable/MeshPresets.h"
#include "Bindable/Material.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"
#include "Core/Macro.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include <memory>
#include <cmath>

namespace Engine { class VoxelWorld; }

namespace Client
{
    // A "Follow" weapon's pet: a small companion that trails the player and
    // fires the weapon at the nearest enemy on its own cooldown. Header-only
    // because it's only ever created via Player::RespawnPets ->
    // CreateGameObject<Pet> (make_shared, no ObjectFactory name lookup), so no
    // REGISTER_GAMEOBJECT is needed. Invulnerable -- no HP / aggro components.
    class Pet : public Engine::GameObject
    {
    public:
        Pet() = default;
        virtual ~Pet() override = default;

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        // Called by Player right after CreateGameObject<Pet>. fRingAngle
        // spreads multiple pets around the player so they don't stack.
        void Configure(int iWeaponId, int iLevel,
                       std::weak_ptr<Engine::Transform> pOwner,
                       float fRingAngle, Engine::VoxelWorld* pVoxel)
        {
            m_iWeaponId   = iWeaponId;
            m_iLevel      = iLevel;
            m_pOwner      = pOwner;
            m_fRingAngle  = fRingAngle;
            m_pVoxelWorld = pVoxel;
            // Tint the pet with the weapon's colour so different Follow weapons
            // are visually distinct.
            if (m_pMaterial)
                if (const WeaponDef* d = WeaponDatabase::GetInst().Get(iWeaponId))
                {
                    const float r = ((d->uColorRGB >> 16) & 0xFF) / 255.f;
                    const float g = ((d->uColorRGB >> 8)  & 0xFF) / 255.f;
                    const float b = ( d->uColorRGB        & 0xFF) / 255.f;
                    m_pMaterial->SetDiffuseColor(r, g, b, 1.f);
                }
        }

        virtual bool Init() override
        {
            if (!__super::Init()) return false;

            m_pTransform    = AddComponent<Engine::Transform>("transform");
            m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
            if (m_pMeshRenderer)
            {
                // Small cube -- same render stack as Tower (STANDARD_VS +
                // STANDARD_SOLID_PS reads the cloned material's diffuse).
                m_pMeshRenderer->SetMesh(Engine::MeshPresets::AxisBox(
                    Engine::Vector3(-0.25f, -0.25f, -0.25f),
                    Engine::Vector3( 0.25f,  0.25f,  0.25f)));
                m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
                m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
                m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
                m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
                if (auto pSrcMat = Engine::StaticFindBindable<Engine::Material>("Material"))
                {
                    m_pMaterial = std::static_pointer_cast<Engine::Material>(pSrcMat->Clone());
                    m_pMaterial->SetDiffuseColor(0.8f, 0.65f, 1.f, 1.f);
                    m_pMaterial->SetTag("PetMat");
                    m_pMeshRenderer->SetMaterial(m_pMaterial);
                    m_pMeshRenderer->SetOverrideMaterial(0, 0, m_pMaterial);
                }
            }
            return true;
        }

        virtual void Update(float fDeltaTime) override
        {
            __super::Update(fDeltaTime);
            auto pOwner = m_pOwner.lock();
            if (!pOwner || !m_pTransform) return;
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iWeaponId);
            if (!pDef) return;

            // Follow: drift toward a ring slot offset from the player at the
            // weapon's speed (a touch faster so it keeps up), so several pets
            // fan out and trail rather than stacking on the player pivot.
            const Engine::Vector3 vOwner = pOwner->GetPosition();
            const Engine::Vector3 vGoal{
                vOwner.x + cosf(m_fRingAngle) * kFollowRadius,
                vOwner.y + kPetYOffset,
                vOwner.z + sinf(m_fRingAngle) * kFollowRadius };
            Engine::Vector3 vPos = m_pTransform->GetPosition();
            const float gx = vGoal.x - vPos.x, gz = vGoal.z - vPos.z;
            const float fDist = sqrtf(gx * gx + gz * gz);
            if (fDist > 0.05f)
            {
                const float fFollowSpeed =
                    (pDef->fProjectileSpeed > 0.f ? pDef->fProjectileSpeed : 4.f) * 1.5f;
                const float fMove = fFollowSpeed * fDeltaTime;
                const float fStep = (fMove < fDist) ? fMove : fDist;
                vPos.x += (gx / fDist) * fStep;
                vPos.z += (gz / fDist) * fStep;
            }
            vPos.y = vOwner.y + kPetYOffset;
            m_pTransform->SetPosition(vPos);

            // Fire on the weapon's cooldown at the nearest enemy in range.
            const float fCd = ComputeCooldown(*pDef, m_iLevel);
            m_fCooldownAcc += fDeltaTime;
            if (m_fCooldownAcc < fCd) return;

            auto pTarget = FindTargetEnemy(vPos, AimMode::Nearest);
            std::shared_ptr<Engine::Transform> pTr =
                pTarget ? pTarget->GetComponent<Engine::Transform>() : nullptr;
            if (!pTr) { m_fCooldownAcc = fCd; return; }   // hold, ready to fire
            const Engine::Vector3 vTgt = pTr->GetPosition();
            const float tx = vTgt.x - vPos.x, tz = vTgt.z - vPos.z;
            if (tx * tx + tz * tz > kRangeSq) { m_fCooldownAcc = fCd; return; }

            m_fCooldownAcc -= fCd;
            FireAt(vTgt, *pDef);
        }

    private:
        void FireAt(const Engine::Vector3& vTarget, const WeaponDef& def)
        {
            auto pScene = GetScene();          if (!pScene) return;
            auto pLayer = pScene->FindLayer(DEFAULT_LAYER); if (!pLayer) return;

            const Engine::Vector3 vPos = m_pTransform->GetPosition();
            const float dx = vTarget.x - vPos.x;
            const float dz = vTarget.z - vPos.z;
            const float fAimYaw = atan2f(-dx, -dz);

            // The pet's bullets must not themselves be Follow (that would spawn
            // a pet that spawns a pet...). Fire them Straight along the locked
            // heading; damage / count / on-hit / spread come from the WeaponDef.
            WeaponDef bulletDef = def;
            bulletDef.eMovement = MovementType::Straight;

            const int iCount = ComputeCount(def, m_iLevel);
            const float fFanStep = 0.174f;   // ~10 deg, matches Player/Tower
            const float fFanBase = -fFanStep * (iCount - 1) * 0.5f;
            for (int i = 0; i < iCount; ++i)
            {
                auto pBullet = pScene->CreateGameObject<Bullet>("bullet", pLayer);
                if (!pBullet) continue;
                pBullet->Configure(bulletDef, m_iLevel, m_pTransform);
                pBullet->SetVoxelWorld(m_pVoxelWorld);
                if (auto pBulletTr = pBullet->GetTransform())
                {
                    pBulletTr->SetPosition(vPos);
                    pBulletTr->SetRX(-PI / 2.f);
                    pBulletTr->SetRY(fAimYaw + fFanBase + fFanStep * i);
                }
            }
        }

        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        std::shared_ptr<Engine::Material>              m_pMaterial;
        std::weak_ptr<Engine::Transform>               m_pOwner;
        Engine::VoxelWorld* m_pVoxelWorld = nullptr;

        int   m_iWeaponId    = -1;
        int   m_iLevel       = 1;
        float m_fCooldownAcc = 0.f;
        float m_fRingAngle   = 0.f;   // this pet's slot around the player ring

        static constexpr float kFollowRadius = 1.6f;   // trail distance
        static constexpr float kPetYOffset   = -0.4f;  // near muzzle height
        static constexpr float kRangeSq      = 12.f * 12.f;   // engage radius^2
    };
}

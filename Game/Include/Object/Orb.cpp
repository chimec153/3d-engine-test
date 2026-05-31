#include "Orb.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Orb, Orb)
#include "Player.h"
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
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "Core/Macro.h"
#include "Types.h"
#include <algorithm>
#include <cmath>

namespace Client
{
    namespace
    {
        // Shared low-poly sphere geometry — every orb in the scene reuses it.
        std::shared_ptr<Engine::Mesh> EnsureOrbMesh()
        {
            return Engine::MeshPresets::UnitSphere(6, 12);
        }

        // One shared material so all orbs sit in the same instancing bucket
        // and the representative material colour matches every member —
        // same rationale as EnsureEnemyMaterial in Enemy.cpp.
        std::shared_ptr<Engine::Material> EnsureOrbMaterial()
        {
            if (auto pCached = Engine::StaticFindBindable<Engine::Material>("OrbMaterial"))
                return pCached;

            auto pMat = Engine::StaticCreateBindable<Engine::Material>("OrbMaterial");
            if (!pMat) return Engine::StaticFindBindable<Engine::Material>("OrbMaterial");

            pMat->SetDiffuseColor (1.0f, 0.85f, 0.2f, 1.f);
            pMat->SetEmissiveColor({ 1.0f, 0.7f, 0.1f, 1.f });
            return pMat;
        }
    }

    Orb::Orb() = default;

    bool Orb::Init()
    {
        if (!__super::Init()) return false;

        m_pTransform    = AddComponent<Engine::Transform>("transform");
        m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

        // Sphere helper emits a radius-0.5 sphere centred at the origin; shrink
        // it via the transform so the orb reads as a small floating bead.
        if (m_pTransform) m_pTransform->SetScale(0.2f, 0.2f, 0.2f);

        auto pMat = EnsureOrbMaterial();

        if (m_pMeshRenderer)
        {
            m_pMeshRenderer->SetMesh(EnsureOrbMesh());
            m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
            m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
            m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
            if (pMat)
            {
                m_pMeshRenderer->SetMaterial(pMat);
                m_pMeshRenderer->SetOverrideMaterial(0, 0, pMat);
            }
        }

        m_pCollider = AddComponent<Engine::ColliderSphere>("orb_body");
        if (m_pCollider)
        {
            // Larger pickup radius — Player::CollisionPlayerBodyStay handles
            // the pickup when its PlayerBody OBB touches this sphere. No
            // callback on the Orb side: putting the handler on the Player
            // body avoids double-counting and dodges any one-way dispatch
            // quirks where the OBB->Sphere check only fires the OBB owner.
            m_pCollider->SetRadius(0.5f);
            // Orbs only need to register against the player.
            m_pCollider->SetGroup(Engine::COLLISION_GROUP::PICKUP);
            m_pCollider->SetMask(Engine::COLLISION_GROUP::PLAYER);
        }

        // Magnet zone — bigger sphere with tag "orb_attract". Same
        // PICKUP×PLAYER pair filter, so the spatial grid + group filter
        // wakes the pair only when the player is in xz range. The STAY
        // callback (orb side) pulls the orb toward the player; the player
        // body's callback ignores this tag (only acts on "orb_body").
        m_pAttractCollider = AddComponent<Engine::ColliderSphere>("orb_attract");
        if (m_pAttractCollider)
        {
            m_pAttractCollider->SetRadius(m_fAttractRadius);
            m_pAttractCollider->SetGroup(Engine::COLLISION_GROUP::PICKUP);
            m_pAttractCollider->SetMask(Engine::COLLISION_GROUP::PLAYER);
            m_pAttractCollider->SetCallBack(Engine::COLLISION_TYPE::STAY,
                this, &Orb::AttractStay);
        }

        return true;
    }

    void Orb::Update(float fDeltaTime)
    {
        __super::Update(fDeltaTime);

        // Round-end magnet: home straight to the player, no range gate. The
        // orb_body pickup collision grants the money once it arrives.
        if (!m_bMagnet || !m_pTransform) return;

        auto pScene = GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;
        auto pPlayer = pLayer->FindGameObject("player");
        if (!pPlayer) return;
        auto pPlayerTr = pPlayer->GetComponent<Engine::Transform>();
        if (!pPlayerTr) return;

        const Engine::Vector3& vOrb    = m_pTransform->GetPosition();
        const Engine::Vector3& vPlayer = pPlayerTr->GetPosition();

        Engine::Vector3 vDelta = vPlayer - vOrb;
        vDelta.y = 0.f;
        const float fDistSq = vDelta.LengthSq();
        if (fDistSq < 1e-6f) return;   // on top — pickup fires

        const float fDist = sqrtf(fDistSq);
        const float fStep = std::min(m_fMagnetSpeed * fDeltaTime, fDist);
        vDelta.Normalize();
        m_pTransform->SetPosition(
            vOrb.x + vDelta.x * fStep,
            vOrb.y,
            vOrb.z + vDelta.z * fStep);
    }

    void Orb::AttractStay(Engine::Collider* /*pSrc*/, Engine::Collider* pDest, float fDeltaTime)
    {
        // pSrc is our attract collider, pDest is whatever it overlaps —
        // expected to be the player body (PICKUP×PLAYER filter).
        if (!m_pTransform || !pDest) return;

        auto* pOwner = pDest->GetGameObjectOwner();
        if (!pOwner) return;
        auto pPlayerTr = pOwner->GetComponent<Engine::Transform>();
        if (!pPlayerTr) return;

        const Engine::Vector3& vOrb    = m_pTransform->GetPosition();
        const Engine::Vector3& vPlayer = pPlayerTr->GetPosition();

        // xz-plane motion: 2D world fixes y per layer, so collapse the y
        // delta to 0 before Vector3::Distance / Normalize. The orb stays
        // on its own y track.
        Engine::Vector3 vDelta = vPlayer - vOrb;
        vDelta.y = 0.f;
        const float fDistSq = vDelta.LengthSq();
        if (fDistSq < 1e-6f) return; // already on top — pickup will fire

        const float fDist = sqrtf(fDistSq);

        // Speed ramps with proximity (1.0 at the magnet edge → ~2.0 at
        // centre) so distant orbs drift in lazily and near orbs snap
        // quickly. Step is clamped to fDist so we never overshoot the
        // player on a single frame.
        const float fT     = 1.f - std::min(fDist / m_fAttractRadius, 1.f);
        const float fSpeed = m_fAttractSpeed * (1.f + fT);
        const float fStep  = std::min(fSpeed * fDeltaTime, fDist);

        // Unit direction × step.
        vDelta.Normalize();
        m_pTransform->SetPosition(
            vOrb.x + vDelta.x * fStep,
            vOrb.y,
            vOrb.z + vDelta.z * fStep);
    }

    void Orb::OnCollision(Engine::Collider* /*pSrc*/, Engine::Collider* /*pDest*/, float /*fDeltaTime*/)
    {
        // Kept for header parity; pickup is driven from Player's body callback.
    }
}

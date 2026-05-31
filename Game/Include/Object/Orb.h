#pragma once
#include "GameObject/GameObject.h"
#include <memory>

namespace Engine
{
    class Transform;
    class MeshRendererComponent;
    class Material;
    class ColliderSphere;
    class Collider;
}

namespace Client
{
    // Pickup orb dropped by dead enemies. Sits at the drop position until a
    // PlayerBody collider touches it, at which point it grants experience to
    // the player GameObject and deactivates itself.
    class Orb : public Engine::GameObject
    {
    public:
        Orb();
        virtual ~Orb() override = default;

        void SetExpValue  (int iExp)   { m_iExp   = iExp;   }
        void SetMoneyValue(int iMoney) { m_iMoney = iMoney; }
        int  GetExpValue  () const     { return m_iExp;   }
        int  GetMoneyValue() const     { return m_iMoney; }

        // Switch on "magnet" mode: from now the orb homes straight to the
        // player every frame regardless of distance (used at round end to
        // auto-collect everything left on the ground). The existing pickup
        // collision grants the money once it reaches the player.
        void PullToPlayer() { m_bMagnet = true; }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;

    private:
        std::shared_ptr<Engine::Transform>             m_pTransform;
        std::shared_ptr<Engine::MeshRendererComponent> m_pMeshRenderer;
        // Two colliders, both PICKUP×PLAYER but different roles:
        //   m_pCollider        — small (radius 0.5), tag "orb_body".
        //                        Player's body OBB pair fires the actual
        //                        pickup (Player::CollisionPlayerBodyStay).
        //   m_pAttractCollider — large (m_fAttractRadius), tag
        //                        "orb_attract". When the player walks
        //                        inside, the STAY callback below pulls
        //                        the orb toward them. Lives entirely on
        //                        the collision system — no per-frame
        //                        distance scan in Update.
        std::shared_ptr<Engine::ColliderSphere>        m_pCollider;
        std::shared_ptr<Engine::ColliderSphere>        m_pAttractCollider;
        int m_iExp   = 1;
        // Money awarded on pickup. Defaulted to the legacy flat constant
        // (GameDefs.h kOrbMoney) so an Orb spawned without SetMoneyValue
        // (e.g. tests) still hands out the previous amount; Enemy
        // overwrites this with the JSON goldReward at death.
        int m_iMoney = 10;

        // Player magnet tuning. Speed ramps by proximity inside AttractStay.
        float m_fAttractRadius = 4.0f;
        float m_fAttractSpeed  = 8.0f;

        // Round-end auto-collect: when true, Update homes the orb to the
        // player at m_fMagnetSpeed with no range limit (collision then picks
        // it up). Set by PullToPlayer().
        bool  m_bMagnet      = false;
        float m_fMagnetSpeed = 22.0f;

        void OnCollision(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
        // STAY callback on the attract collider — fires every frame the
        // player overlaps the magnet sphere. Pulls the orb toward the
        // player's host transform position.
        void AttractStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime);
    };
}

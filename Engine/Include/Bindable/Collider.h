#pragma once
#include "../Component/Component.h"
#include "../Types.h"
#include <vector>
namespace Engine
{
    // Phase B.4 — Collider migrated from Bindable to Component. Pure
    // collision-detection logic; debug-visualization Drawables (used by
    // OBB/Sphere etc.) are owned as direct members and re-parented onto
    // the owning Drawable's m_ChildList<Bindable> when the Collider is
    // attached, so the existing debug-render path keeps working.
    class ENGINE_DLL Collider :
        public Component
    {
    protected:
        Collider();
        Collider(const Collider& collider);
        virtual ~Collider() override = default;

    private:
        COLLIDER_TYPE   m_eColliderType;
        Vector3     m_vCross;
        // Contiguous flat container. Each Collider's "previously
        // collided this/last frame" set is tiny in practice (0–5 entries
        // for typical entities), where a vector linear scan beats
        // std::unordered_set::find — the set's hash + bucket array +
        // node allocation overhead dwarfs N pointer compares from a
        // contiguous cache line. (Original std::list was worse still:
        // each entry a separate heap node, terrible cache behaviour.)
        std::vector<class Collider*> m_PrevColliderList;
        std::function<void(Collider*, Collider*, float)>    m_CallBack[static_cast<int>(COLLISION_TYPE::END)];
        COLLISION_CHANNEL m_eChannel;

        // Object-type pair-filter bits. Defaults preserve "collide with
        // everything" — only colliders set up by game code that should
        // skip irrelevant pairs (bullet↔bullet, orb↔enemy, ...) need to
        // override these. See COLLISION_GROUP in Flag.h.
        COLLISION_GROUP m_eGroup = COLLISION_GROUP::DEFAULT;
        COLLISION_GROUP m_eMask  = COLLISION_GROUP::ALL;

    public:
        // Phase E7 — debug visualization Drawable removed. Live entities are
        // GameObjects now, so the "re-parent debug drawable onto owning
        // Drawable's Bindable child list" mechanism (which Phase B.4 relied
        // on) has no live host. A future debug-renderer would attach to a
        // GameObject + MeshRendererComponent, or live in RenderManager
        // directly as an immediate-mode pass.

    public:
        const COLLIDER_TYPE GetColliderType()   const;
        void SetColliderType(COLLIDER_TYPE eType);
        const Vector3& GetCross()  const;
        void SetCross(const Vector3& vCross);
        void AddPrevCollider(class Collider* pCollider);
        bool HasPrevCollider(class Collider* pCollider) const;
        void DeletePrevCollider(class Collider* pCollider);
        template <typename T>
        void SetCallBack(COLLISION_TYPE eType, T* pObject, void(T::* pFunc)(Collider*, Collider*, float))
        {
            assert(static_cast<int>(eType) >= 0 && eType < COLLISION_TYPE::END);

            m_CallBack[static_cast<int>(eType)] = std::bind(pFunc, pObject, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        }
        void SetCallBack(COLLISION_TYPE eType, void(*pFunc)(Collider*, Collider*, float));
        void Call(COLLISION_TYPE eType, Collider* pDest, float fDeltaTime);
        const std::vector<class Collider*>& GetPrevColliderList() const;
        void ClearCallBack();
        void SetChannel(COLLISION_CHANNEL eChannel);
        COLLISION_CHANNEL GetChannel()  const noexcept;

        // Object-type pair-filter accessors. CollisionManager rejects a
        // pair (A, B) before any virtual dispatch when
        //   !(A.group & B.mask) || !(B.group & A.mask)
        // so this is one cheap bitwise AND per pair vs the per-pair
        // virtual call / HasPrevCollider scan it replaces.
        void SetGroup(COLLISION_GROUP eGroup) { m_eGroup = eGroup; }
        void SetMask(COLLISION_GROUP  eMask)  { m_eMask  = eMask;  }
        COLLISION_GROUP GetGroup() const noexcept { return m_eGroup; }
        COLLISION_GROUP GetMask()  const noexcept { return m_eMask;  }

    public:
        virtual void Collision(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime) = 0;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
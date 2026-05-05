#pragma once
#include "../Component/Component.h"
#include "../Types.h"
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
        std::list<class Collider*> m_PrevColliderList;
        std::function<void(Collider*, Collider*, float)>    m_CallBack[static_cast<int>(COLLISION_TYPE::END)];
        COLLISION_CHANNEL m_eChannel;

    protected:
        // Phase B.4 — debug visualization Drawable (created in the subtype
        // ctor for OBB/Sphere; null for Line/Mesh which don't draw).
        // Drawable::AddChild(Component) inspects this and, if non-null,
        // re-parents it onto the owning Drawable's Bindable child list.
        std::shared_ptr<class Drawable> m_pDebugDrawable;
    public:
        std::shared_ptr<class Drawable> GetDebugDrawable() const { return m_pDebugDrawable; }

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
        const std::list<class Collider*>& GetPrevColliderList() const;
        void ClearCallBack();
        void SetChannel(COLLISION_CHANNEL eChannel);
        COLLISION_CHANNEL GetChannel()  const noexcept;

    public:
        virtual void Collision(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime) = 0;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
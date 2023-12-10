#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL Collider :
        public Bindable
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

    public:
        virtual void Collision(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime) = 0;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;

    };

}
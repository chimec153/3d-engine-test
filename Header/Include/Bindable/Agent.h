#pragma once
#include "Bindable.h"

namespace Engine
{
    class ENGINE_DLL Agent :
        public Bindable
    {
    public:
        Agent(std::shared_ptr<class Transform> pTransform, class NavMesh* pNavMesh, const Vector3& pos);
        Agent(const Agent& agent);
        virtual ~Agent() override = default;

    private:
        std::shared_ptr<class Transform> m_pTransform;
        class NavMesh* m_pNavMesh;
        int m_iAgentIndex;

    public:
        void SetTargetPos(const Vector3& pos);
        const Vector3 GetAgentVelocity()  const;
        int CreateAgent(const Vector3& pos);
        void SetTransform(std::shared_ptr<Transform> pTransform);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
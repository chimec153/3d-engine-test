#pragma once
#include "../Component/Component.h"
#include "../Navigation/Detour/DetourCrowd.h"

namespace Engine
{
    // Phase B.4 — Agent migrated from Bindable to Component. Pure
    // CPU navigation logic; no GPU bindings. Bind() was a no-op.
    class ENGINE_DLL Agent :
        public Component
    {
    public:
        Agent();
        Agent(std::shared_ptr<class Transform> pTransform, std::weak_ptr<class NavMesh> pNavMesh, const Vector3& pos);
        Agent(const Agent& agent);
        virtual ~Agent() override;

    private:
        std::shared_ptr<class Transform> m_pTransform;
        std::weak_ptr<NavMesh> m_pNavMesh;
        int m_iAgentIndex;
        std::unique_ptr<dtCrowdAgentParams> m_pCrowdParams;

    public:
        void SetTargetPos(const Vector3& pos);
        const Vector3 GetAgentVelocity()  const;
        int CreateAgent(const Vector3& pos);
        void SetTransform(std::shared_ptr<Transform> pTransform);
        void SetNavMesh(std::weak_ptr<NavMesh> pNavMesh);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}
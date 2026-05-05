#pragma once
#include "../Component/Component.h"
#include "../Navigation/Detour/DetourNavMeshBuilder.h"
#include "../Navigation/Detour/DetourCrowd.h"

class dtNavMesh;
class dtNavMeshQuery;
class dtCrowd;

namespace Engine
{
    class Agent;

    // Phase B.4 — NavMesh migrated from Bindable to Component. Pure
    // navigation data structure; no GPU bindings. Bind() was a no-op.
    class ENGINE_DLL NavMesh :
        public Component
    {
    public:
        NavMesh(dtNavMeshCreateParams& tParam, float fAgentRadius, float fAgentHeight);
        NavMesh();
        virtual ~NavMesh() override;

    private:
        class dtNavMesh* m_pNavMesh;
        class dtNavMeshQuery* m_pNavMeshQuery;
        unsigned char* m_pNavData;
        int m_iNavCount;
        float m_fAgentRadius;
        float m_fAgentHeight;
        class dtCrowd* m_pCrowd;
        std::unique_ptr<dtNavMeshCreateParams> m_pMeshCreateParams;
        std::list<std::shared_ptr<Agent>> m_AgentList;

    public:
        class dtNavMesh* GetNavMesh() const;
        int CreateAgent(const Vector3& pos, dtCrowdAgentParams& tParams);
        Vector3 GetAgentPos(int iIndex) const;
        Vector3 GetAgentVelocity(int iIndex) const;
        void SetTargetPos(int iIndex, const Vector3& pos);
        void DeleteAgent(int iIndex);
        void CreateNavMesh(dtNavMeshCreateParams& tParams);
        std::shared_ptr<Agent> CreateAgent(const std::string& strTag, std::shared_ptr<Transform> pTransform, const Vector3& vPos);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };


}
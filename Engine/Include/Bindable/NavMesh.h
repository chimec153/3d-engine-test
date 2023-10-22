#pragma once
#include "Bindable.h"
#include "../Navigation/Detour/DetourNavMeshBuilder.h"

class dtNavMesh;
class dtNavMeshQuery;
class dtCrowd;

namespace Engine
{
    class Agent;

    class ENGINE_DLL NavMesh :
        public Bindable
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

    public:
        class dtNavMesh* GetNavMesh() const;
        int CreateAgent(const Vector3& pos);
        Vector3 GetAgentPos(int iIndex) const;
        Vector3 GetAgentVelocity(int iIndex) const;
        void SetTargetPos(int iIndex, const Vector3& pos);
        void DeleteAgent(int iIndex);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };


}
#include "NavMesh.h"
#include "../Navigation/Detour/DetourNavMesh.h"
#include "../Navigation/Detour/DetourNavMeshQuery.h"
#include "Agent.h"
#include "../Navigation/Detour/DetourCrowd.h"

Engine::NavMesh::NavMesh(dtNavMeshCreateParams& tParams, float fAgentRadius, float fAgentHeight) :
	Bindable()
	, m_pNavMesh(nullptr)
	, m_pNavMeshQuery(nullptr)
	, m_pNavData(nullptr)
	, m_iNavCount(0)
	, m_fAgentRadius(fAgentRadius)
	, m_fAgentHeight(fAgentHeight)
	, m_pCrowd(dtAllocCrowd())
{
	SetBindableType(BINDABLE_TYPE::NAV_MESH);

	if (!dtCreateNavMeshData(&tParams, &m_pNavData, &m_iNavCount))
	{
		return;
	}

	m_pNavMesh = dtAllocNavMesh();

	if (!m_pNavMesh)
	{
		return;
	}

	dtStatus iStatus = m_pNavMesh->init(m_pNavData, m_iNavCount, DT_TILE_FREE_DATA);

	if (dtStatusFailed(iStatus))
	{
		return;
	}

	m_pNavMeshQuery = dtAllocNavMeshQuery();

	if (!m_pNavMeshQuery)
	{
		return;
	}

	iStatus = m_pNavMeshQuery->init(m_pNavMesh, 2048);

	if (dtStatusFailed(iStatus))
	{
		return;
	}
	if (!m_pCrowd)
	{
		return;
	}

	if (!m_pCrowd->init(128, m_fAgentRadius, m_pNavMesh))
	{
		return;
	}

	m_pCrowd->getEditableFilter(0)->setExcludeFlags(0x10);

	dtObstacleAvoidanceParams tObstacleParams;

	memcpy_s(&tObstacleParams, sizeof(dtObstacleAvoidanceParams), m_pCrowd->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));

	tObstacleParams.velBias = 0.5f;
	tObstacleParams.adaptiveDivs = 5;
	tObstacleParams.adaptiveRings = 2;
	tObstacleParams.adaptiveDepth = 1;
	m_pCrowd->setObstacleAvoidanceParams(0, &tObstacleParams);

	tObstacleParams.velBias = 0.5f;
	tObstacleParams.adaptiveDivs = 5;
	tObstacleParams.adaptiveRings = 2;
	tObstacleParams.adaptiveDepth = 2;
	m_pCrowd->setObstacleAvoidanceParams(1, &tObstacleParams);

	tObstacleParams.velBias = 0.5f;
	tObstacleParams.adaptiveDivs = 7;
	tObstacleParams.adaptiveRings = 2;
	tObstacleParams.adaptiveDepth = 3;
	m_pCrowd->setObstacleAvoidanceParams(2, &tObstacleParams);

	tObstacleParams.velBias = 0.5f;
	tObstacleParams.adaptiveDivs = 7;
	tObstacleParams.adaptiveRings = 3;
	tObstacleParams.adaptiveDepth = 3;
	m_pCrowd->setObstacleAvoidanceParams(3, &tObstacleParams);
}

Engine::NavMesh::NavMesh() :
	Bindable()
	, m_pNavMesh(nullptr)
	, m_pNavMeshQuery(nullptr)
	, m_pNavData(nullptr)
	, m_iNavCount(0)
{
}

Engine::NavMesh::~NavMesh()
{
	if (m_pNavMesh)
	{
		dtFreeNavMesh(m_pNavMesh);
		m_pNavMesh = nullptr;
	}

	if (m_pNavMeshQuery)
	{
		dtFreeNavMeshQuery(m_pNavMeshQuery);
		m_pNavMeshQuery = nullptr;
	}

	if (m_pCrowd)
	{
		dtFreeCrowd(m_pCrowd);
		m_pCrowd = nullptr;
	}
}

void Engine::NavMesh::Bind()
{
}

int Engine::NavMesh::CreateAgent(const Vector3& pos)
{
	dtCrowdAgentParams tParams = {};

	tParams.radius = m_fAgentRadius;
	tParams.height = m_fAgentHeight;
	tParams.maxAcceleration = 8.0f;
	tParams.maxSpeed = 2.0f;
	tParams.collisionQueryRange = tParams.radius * 12.f;
	tParams.pathOptimizationRange = tParams.radius * 30.f;
	tParams.updateFlags = 0;
	tParams.obstacleAvoidanceType = 0;
	tParams.separationWeight = 0.f;

	return m_pCrowd->addAgent(&pos.x, &tParams);
}

Engine::Vector3 Engine::NavMesh::GetAgentPos(int iIndex) const
{
	const dtCrowdAgent* pAgent = m_pCrowd->getAgent(iIndex);

	if (!pAgent)
	{
		return Vector3();
	}

	return Vector3(pAgent->npos[0], pAgent->npos[1], pAgent->npos[2]);
}

Engine::Vector3 Engine::NavMesh::GetAgentVelocity(int iIndex)	const
{
	const dtCrowdAgent* pAgent = m_pCrowd->getAgent(iIndex);

	if (!pAgent)
	{
		return Engine::Vector3();
	}

	return Vector3(pAgent->vel);
}

void Engine::NavMesh::SetTargetPos(int iIndex, const Vector3& pos)
{
	dtPolyRef iRef;

	float nearPos[3];

	dtStatus iStatus = m_pNavMeshQuery->findNearestPoly(&pos.x, m_pCrowd->getQueryExtents(), m_pCrowd->getFilter(0), &iRef, nearPos);

	if (iRef)
	{
		m_pCrowd->requestMoveTarget(iIndex, iRef, nearPos);
	}
}

void Engine::NavMesh::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	if (m_pCrowd)
	{
		m_pCrowd->update(fDeltaTime, nullptr);
	}
}

void Engine::NavMesh::DeleteAgent(int iIndex)
{
	if (m_pCrowd)
	{
		m_pCrowd->removeAgent(iIndex);
	}
}

std::shared_ptr < Engine::Bindable > Engine::NavMesh::Clone()
{
	return std::shared_ptr<Bindable>();
}

dtNavMesh* Engine::NavMesh::GetNavMesh() const
{
	return m_pNavMesh;
}

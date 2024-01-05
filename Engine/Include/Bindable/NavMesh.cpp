#include "NavMesh.h"
#include "../Navigation/Detour/DetourNavMesh.h"
#include "../Navigation/Detour/DetourNavMeshQuery.h"
#include "Agent.h"
#include "../Navigation/Detour/DetourCrowd.h"
#include "Transform.h"

Engine::NavMesh::NavMesh(dtNavMeshCreateParams& tParams, float fAgentRadius, float fAgentHeight) :
	Bindable()
	, m_pNavMesh(dtAllocNavMesh())
	, m_pNavMeshQuery(dtAllocNavMeshQuery())
	, m_pNavData(nullptr)
	, m_iNavCount(0)
	, m_fAgentRadius(fAgentRadius)
	, m_fAgentHeight(fAgentHeight)
	, m_pCrowd(dtAllocCrowd())
	, m_pMeshCreateParams(std::make_unique<dtNavMeshCreateParams>())
{
	*m_pMeshCreateParams = tParams;

	unsigned short* pVerts = dbg_new unsigned short[m_pMeshCreateParams->vertCount * 3];

	memcpy_s(pVerts, m_pMeshCreateParams->vertCount * 6, tParams.verts, m_pMeshCreateParams->vertCount * 6);

	unsigned char* pPolyAreas = dbg_new unsigned char[m_pMeshCreateParams->polyCount];

	memcpy_s(pPolyAreas, m_pMeshCreateParams->polyCount, tParams.polyAreas, m_pMeshCreateParams->polyCount);

	unsigned short* pPolys = dbg_new unsigned short[m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount * 2];

	memcpy_s(pPolys, m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount * 4, tParams.polys, m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount * 4);

	unsigned short* pPolyFlags = dbg_new unsigned short[m_pMeshCreateParams->polyCount];

	memcpy_s(pPolyFlags, m_pMeshCreateParams->polyCount * 2, tParams.polyFlags, m_pMeshCreateParams->polyCount * 2);

	m_pMeshCreateParams->verts = pVerts;

	m_pMeshCreateParams->polyAreas = pPolyAreas;

	m_pMeshCreateParams->polys = pPolys;

	m_pMeshCreateParams->polyFlags = pPolyFlags;

	SetBindableType(BINDABLE_TYPE::NAV_MESH);

	CreateNavMesh(*m_pMeshCreateParams);
}

Engine::NavMesh::NavMesh() :
	Bindable()
	, m_pNavMesh(dtAllocNavMesh())
	, m_pNavMeshQuery(dtAllocNavMeshQuery())
	, m_pNavData(nullptr)
	, m_iNavCount(0)
	, m_fAgentRadius(0.f)
	, m_fAgentHeight(0.f)
	, m_pCrowd(dtAllocCrowd())
	, m_pMeshCreateParams(std::make_unique<dtNavMeshCreateParams>())
{
	SetBindableType(BINDABLE_TYPE::NAV_MESH);
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

	if (m_pMeshCreateParams)
	{
		SAFE_DELETE_ARRAY(m_pMeshCreateParams->verts);
		SAFE_DELETE_ARRAY(m_pMeshCreateParams->polyAreas);
		SAFE_DELETE_ARRAY(m_pMeshCreateParams->polys);
		SAFE_DELETE_ARRAY(m_pMeshCreateParams->polyFlags);
	}
}

void Engine::NavMesh::Bind()
{
}

int Engine::NavMesh::CreateAgent(const Vector3& pos, dtCrowdAgentParams& tParams)
{
	tParams.radius = m_fAgentRadius;
	tParams.height = m_fAgentHeight;
	tParams.collisionQueryRange = tParams.radius * 12.f;
	tParams.pathOptimizationRange = tParams.radius * 30.f;

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

void Engine::NavMesh::CreateNavMesh(dtNavMeshCreateParams& tParams)
{
	if (!m_pCrowd)
	{
		assert(false);
		return;
	}

	if (!m_pNavMesh)
	{
		assert(false);
		return;
	}

	if (!m_pNavMeshQuery)
	{
		assert(false);
		return;
	}

	if (!dtCreateNavMeshData(&tParams, &m_pNavData, &m_iNavCount))
	{
		assert(false);
		return;
	}

	dtStatus iStatus = m_pNavMesh->init(m_pNavData, m_iNavCount, DT_TILE_FREE_DATA);

	if (dtStatusFailed(iStatus))
	{
		assert(false);
		return;
	}

	iStatus = m_pNavMeshQuery->init(m_pNavMesh, 2048);

	if (dtStatusFailed(iStatus))
	{
		assert(false);
		return;
	}

	if (!m_pCrowd->init(128, m_fAgentRadius, m_pNavMesh))
	{
		assert(false);
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

std::shared_ptr<Engine::Agent> Engine::NavMesh::CreateAgent(const std::string& strTag, std::shared_ptr<Engine::Transform> pTransform, const Vector3& vPos)
{
	std::shared_ptr<Engine::Agent> pAgent = CreateBindable<Agent>(strTag, pTransform, std::static_pointer_cast<NavMesh>(std::shared_ptr<CRef>(weak_from_this())), vPos);

	m_AgentList.push_back(pAgent);

	return pAgent;
}

std::shared_ptr < Engine::Bindable > Engine::NavMesh::Clone()
{
	return std::shared_ptr<Bindable>();
}

void Engine::NavMesh::Save(FILE* pFile)
{
	__super::Save(pFile);

	fwrite(&m_iNavCount, 4, 1, pFile);
	fwrite(&m_fAgentRadius, 4, 1, pFile);
	fwrite(&m_fAgentHeight, 4, 1, pFile);

	fwrite(&m_pMeshCreateParams->vertCount, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->polyCount, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->nvp, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->userId, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->tileX, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->tileY, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->tileLayer, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->bmin, 4, 3, pFile);
	fwrite(&m_pMeshCreateParams->bmax, 4, 3, pFile);
	fwrite(&m_pMeshCreateParams->walkableHeight, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->walkableRadius, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->walkableClimb, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->cs, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->ch, 4, 1, pFile);
	fwrite(&m_pMeshCreateParams->buildBvTree, 1, 1, pFile);

	fwrite(m_pMeshCreateParams->verts, 2, 3 * m_pMeshCreateParams->vertCount, pFile);
	fwrite(m_pMeshCreateParams->polys, 2, 2 * m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount, pFile);
	fwrite(m_pMeshCreateParams->polyAreas, 1, m_pMeshCreateParams->polyCount, pFile);
	fwrite(m_pMeshCreateParams->polyFlags, 2, m_pMeshCreateParams->polyCount, pFile);
}

void Engine::NavMesh::Load(FILE* pFile)
{
	__super::Load(pFile);

	fread(&m_iNavCount, 4, 1, pFile);
	fread(&m_fAgentRadius, 4, 1, pFile);
	fread(&m_fAgentHeight, 4, 1, pFile);

	fread(&m_pMeshCreateParams->vertCount, 4, 1, pFile);
	fread(&m_pMeshCreateParams->polyCount, 4, 1, pFile);
	fread(&m_pMeshCreateParams->nvp, 4, 1, pFile);
	fread(&m_pMeshCreateParams->userId, 4, 1, pFile);
	fread(&m_pMeshCreateParams->tileX, 4, 1, pFile);
	fread(&m_pMeshCreateParams->tileY, 4, 1, pFile);
	fread(&m_pMeshCreateParams->tileLayer, 4, 1, pFile);
	fread(&m_pMeshCreateParams->bmin, 4, 3, pFile);
	fread(&m_pMeshCreateParams->bmax, 4, 3, pFile);
	fread(&m_pMeshCreateParams->walkableHeight, 4, 1, pFile);
	fread(&m_pMeshCreateParams->walkableRadius, 4, 1, pFile);
	fread(&m_pMeshCreateParams->walkableClimb, 4, 1, pFile);
	fread(&m_pMeshCreateParams->cs, 4, 1, pFile);
	fread(&m_pMeshCreateParams->ch, 4, 1, pFile);
	fread(&m_pMeshCreateParams->buildBvTree, 1, 1, pFile);

	unsigned short* pVerts = dbg_new unsigned short[m_pMeshCreateParams->vertCount * 3];

	fread(pVerts, 2, m_pMeshCreateParams->vertCount * 3, pFile);

	unsigned short* pPolys = dbg_new unsigned short[m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount * 2];

	fread(pPolys, 2, m_pMeshCreateParams->nvp * m_pMeshCreateParams->polyCount * 2, pFile);

	unsigned char* pPolyAreas = dbg_new unsigned char[m_pMeshCreateParams->polyCount];

	fread(pPolyAreas, 1, m_pMeshCreateParams->polyCount, pFile);

	unsigned short* pPolyFlags = dbg_new unsigned short[m_pMeshCreateParams->polyCount];

	fread(pPolyFlags, 2, m_pMeshCreateParams->polyCount, pFile);

	m_pMeshCreateParams->verts = pVerts;

	m_pMeshCreateParams->polyAreas = pPolyAreas;

	m_pMeshCreateParams->polys = pPolys;

	m_pMeshCreateParams->polyFlags = pPolyFlags;

	CreateNavMesh(*m_pMeshCreateParams);

	std::vector<std::shared_ptr<Bindable>> vecAgent;

	FindChilds(BINDABLE_TYPE::AGENT, vecAgent);

	for (size_t i = 0; i < vecAgent.size(); ++i)
	{
		std::static_pointer_cast<Agent>(vecAgent[i])->SetNavMesh(std::static_pointer_cast<NavMesh>(std::shared_ptr<CRef>(weak_from_this())));
	}
}

dtNavMesh* Engine::NavMesh::GetNavMesh() const
{
	return m_pNavMesh;
}

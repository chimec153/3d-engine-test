#include "NavMesh.h"
#include "../Navigation/Detour/DetourNavMesh.h"
#include "../Navigation/Detour/DetourNavMeshQuery.h"
#include "Agent.h"
#include "../Navigation/Detour/DetourCrowd.h"
#include "Transform.h"
#include "../Navigation/Recast/Recast.h"
#include "Mesh.h"

Engine::NavMesh::NavMesh(dtNavMeshCreateParams& tParams, float fAgentRadius, float fAgentHeight) :
	Component()
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

	SetComponentType(COMPONENT_TYPE::NAV_MESH);

	CreateNavMesh(*m_pMeshCreateParams);
}

Engine::NavMesh::NavMesh() :
	Component()
	, m_pNavMesh(dtAllocNavMesh())
	, m_pNavMeshQuery(dtAllocNavMeshQuery())
	, m_pNavData(nullptr)
	, m_iNavCount(0)
	, m_fAgentRadius(0.f)
	, m_fAgentHeight(0.f)
	, m_pCrowd(dtAllocCrowd())
	, m_pMeshCreateParams(std::make_unique<dtNavMeshCreateParams>())
{
	SetComponentType(COMPONENT_TYPE::NAV_MESH);
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
	// Phase B.4 — Agent is now a Component, not a Bindable child of NavMesh.
	// CreateBindable<Agent> path no longer compiles (Agent ∉ Bindable).
	// We track Agents via m_AgentList (already authoritative); the old
	// child-list registration was redundant.
	std::shared_ptr<Engine::Agent> pAgent = std::make_shared<Agent>(
		pTransform,
		std::static_pointer_cast<NavMesh>(std::shared_ptr<CRef>(weak_from_this())),
		vPos);
	pAgent->SetTag(strTag);
	pAgent->Init();

	m_AgentList.push_back(pAgent);

	return pAgent;
}

std::shared_ptr<Engine::Component> Engine::NavMesh::Clone()
{
	return std::shared_ptr<Component>();
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

	// Phase B.4 — Agents are now tracked via m_AgentList (Components),
	// not the old Bindable child-list FindChilds(BINDABLE_TYPE::AGENT).
	for (auto& pAgent : m_AgentList)
	{
		pAgent->SetNavMesh(std::static_pointer_cast<NavMesh>(std::shared_ptr<CRef>(weak_from_this())));
	}
}

dtNavMesh* Engine::NavMesh::GetNavMesh() const
{
	return m_pNavMesh;
}

namespace Engine
{
	namespace
	{
		// RAII guard for Recast intermediates. Freed in reverse-allocation
		// order on scope exit (success or early return), so every error
		// path in Build() doesn't have to repeat the cleanup.
		struct RecastIntermediates
		{
			rcHeightfield*        pHF  = nullptr;
			rcCompactHeightfield* pCHF = nullptr;
			rcContourSet*         pCS  = nullptr;
			rcPolyMesh*           pPM  = nullptr;
			rcPolyMeshDetail*     pPMD = nullptr;

			~RecastIntermediates()
			{
				// Detour's NavMesh constructor copies everything it needs
				// out of dtNavMeshCreateParams, so freeing here is safe.
				if (pPMD) rcFreePolyMeshDetail(pPMD);
				if (pPM)  rcFreePolyMesh(pPM);
				if (pCS)  rcFreeContourSet(pCS);
				if (pCHF) rcFreeCompactHeightfield(pCHF);
				if (pHF)  rcFreeHeightField(pHF);
			}
		};
	}

	std::shared_ptr<NavMesh> NavMesh::Build(
		const std::vector<float>& vecPoint,
		const std::vector<int>& vecTris,
		const Vector3& vMax,
		const Vector3& vMin,
		const NavMeshConfig& cfg)
	{
		if (vecPoint.empty() || vecTris.empty()) return nullptr;

		rcContext tContext;
		RecastIntermediates guard;

		rcConfig config = {};
		memcpy_s(config.bmax, 12, &vMax.x, 12);
		memcpy_s(config.bmin, 12, &vMin.x, 12);
		config.cs = cfg.fCellSize;
		config.ch = cfg.fCellHeight;
		config.walkableSlopeAngle = cfg.fAgentSlopeAngle;
		config.walkableHeight = static_cast<int>(ceilf(cfg.fAgentHeight / config.ch));
		config.walkableRadius = static_cast<int>(ceilf(cfg.fAgentRadius / config.cs));
		config.walkableClimb  = static_cast<int>(floorf(cfg.fAgentClimb / config.ch));
		config.maxEdgeLen = static_cast<int>(cfg.fMaxEdgeLen / cfg.fCellSize);
		config.maxSimplificationError = cfg.fMaxEdgeError;
		config.minRegionArea  = static_cast<int>(cfg.fRegionMinSize * cfg.fRegionMinSize);
		config.mergeRegionArea = static_cast<int>(cfg.fRegionMergeSize * cfg.fRegionMergeSize);
		config.maxVertsPerPoly = static_cast<int>(cfg.fVertsPerPoly);
		config.borderSize = config.walkableRadius + 3;
		rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);
		config.detailSampleDist = cfg.fDetailSampleDist < 0.9f ? 0.f : cfg.fCellSize * cfg.fDetailSampleDist;
		config.detailSampleMaxError = cfg.fCellHeight * cfg.fDetailSampleMaxError;

		config.bmin[0] -= config.borderSize * config.cs;
		config.bmin[2] -= config.borderSize * config.cs;
		config.bmax[0] += config.borderSize * config.cs;
		config.bmax[2] += config.borderSize * config.cs;

		guard.pHF = rcAllocHeightfield();
		if (!guard.pHF) return nullptr;
		if (!rcCreateHeightfield(&tContext, *guard.pHF, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
			return nullptr;

		const int iTriCount = static_cast<int>(vecTris.size() / 3);
		auto pTriAreas = std::make_unique<unsigned char[]>(iTriCount);
		memset(pTriAreas.get(), 0, iTriCount);

		rcMarkWalkableTriangles(&tContext, cfg.fAgentSlopeAngle, &vecPoint[0], static_cast<int>(vecPoint.size()),
			&vecTris[0], iTriCount, pTriAreas.get());

		if (!rcRasterizeTriangles(&tContext, &vecPoint[0], static_cast<int>(vecPoint.size()),
			&vecTris[0], pTriAreas.get(), iTriCount, *guard.pHF, config.walkableClimb))
			return nullptr;

		guard.pCHF = rcAllocCompactHeightfield();
		if (!guard.pCHF) return nullptr;
		if (!rcBuildCompactHeightfield(&tContext, config.walkableHeight, config.walkableClimb, *guard.pHF, *guard.pCHF))
			return nullptr;
		if (!rcErodeWalkableArea(&tContext, config.walkableRadius, *guard.pCHF)) return nullptr;
		if (!rcBuildDistanceField(&tContext, *guard.pCHF)) return nullptr;
		if (!rcBuildRegions(&tContext, *guard.pCHF, 0, config.minRegionArea, config.mergeRegionArea)) return nullptr;

		guard.pCS = rcAllocContourSet();
		if (!guard.pCS) return nullptr;
		if (!rcBuildContours(&tContext, *guard.pCHF, config.maxSimplificationError, config.maxEdgeLen, *guard.pCS))
			return nullptr;

		guard.pPM = rcAllocPolyMesh();
		if (!guard.pPM) return nullptr;
		if (!rcBuildPolyMesh(&tContext, *guard.pCS, config.maxVertsPerPoly, *guard.pPM)) return nullptr;

		guard.pPMD = rcAllocPolyMeshDetail();
		if (!guard.pPMD) return nullptr;
		if (!rcBuildPolyMeshDetail(&tContext, *guard.pPM, *guard.pCHF, config.detailSampleDist, config.detailSampleMaxError, *guard.pPMD))
			return nullptr;

		// Mark walkable polys as area 0 / flag 1 so Detour treats them
		// as the default traversable surface (mirrors the editor's pass).
		for (int i = 0; i < guard.pPM->npolys; ++i)
		{
			if (guard.pPM->areas[i] == RC_WALKABLE_AREA) guard.pPM->areas[i] = 0;
			if (guard.pPM->areas[i] == 0)                guard.pPM->flags[i] = 1;
		}

		dtNavMeshCreateParams tParams = {};
		tParams.verts            = guard.pPM->verts;
		tParams.vertCount        = guard.pPM->nverts;
		tParams.polys            = guard.pPM->polys;
		tParams.polyAreas        = guard.pPM->areas;
		tParams.polyFlags        = guard.pPM->flags;
		tParams.polyCount        = guard.pPM->npolys;
		tParams.nvp              = guard.pPM->nvp;
		tParams.detailMeshes     = guard.pPMD->meshes;
		tParams.detailVerts      = guard.pPMD->verts;
		tParams.detailVertsCount = guard.pPMD->nverts;
		tParams.detailTris       = guard.pPMD->tris;
		tParams.detailTriCount   = guard.pPMD->ntris;
		tParams.walkableHeight   = cfg.fAgentHeight;
		tParams.walkableRadius   = cfg.fAgentRadius;
		tParams.walkableClimb    = cfg.fAgentClimb;
		memcpy_s(tParams.bmin, 12, guard.pPM->bmin, 12);
		memcpy_s(tParams.bmax, 12, guard.pPM->bmax, 12);
		tParams.cs               = config.cs;
		tParams.ch               = config.ch;
		tParams.buildBvTree      = true;

		// NavMesh ctor copies everything it needs; guard frees the Recast
		// intermediates on scope exit.
		return std::make_shared<NavMesh>(tParams, cfg.fAgentRadius, cfg.fAgentHeight);
	}

	std::shared_ptr<Mesh> NavMesh::CreateDebugMesh() const
	{
		if (!m_pNavMesh) return nullptr;

		// dtNavMesh has both a public `const dtMeshTile* getTile(int) const`
		// and a private `dtMeshTile* getTile(int)`. Calling through a
		// non-const dtNavMesh* picks the private overload — link error.
		// Routing the call through a const pointer forces the public one.
		const dtNavMesh* pNav = m_pNavMesh;

		std::vector<VertexStandard> vecVertex;
		std::vector<unsigned int>   vecIndex;

		const int iMaxTiles = pNav->getMaxTiles();
		for (int t = 0; t < iMaxTiles; ++t)
		{
			const dtMeshTile* pTile = pNav->getTile(t);
			if (!pTile || !pTile->header) continue;

			// Each tile owns its own vertex array; index polys into it
			// after offsetting by what we've already appended.
			const int iVertBase = static_cast<int>(vecVertex.size());

			for (int v = 0; v < pTile->header->vertCount; ++v)
			{
				VertexStandard vert;
				vert.pos.x = pTile->verts[v * 3 + 0];
				vert.pos.y = pTile->verts[v * 3 + 1];
				vert.pos.z = pTile->verts[v * 3 + 2];
				// Default up-pointing normal — debug mesh is rendered with
				// WIREFRAME rasterizer + the standard nav shader, neither
				// of which actually shades, but the input layout still
				// requires a normal value.
				vert.normal.y = 1.f;
				vert.tangent.x = 1.f;
				vert.tangent.w = 1.f;
				vert.blendWeight.x = 1.f;
				vecVertex.push_back(vert);
			}

			for (int p = 0; p < pTile->header->polyCount; ++p)
			{
				const dtPoly& poly = pTile->polys[p];
				// Skip off-mesh connections — they're 2-vertex jump links,
				// not surface polys.
				if (poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

				// Fan-triangulate the polygon (Detour guarantees convex).
				for (int i = 1; i + 1 < poly.vertCount; ++i)
				{
					vecIndex.push_back(iVertBase + poly.verts[0]);
					vecIndex.push_back(iVertBase + poly.verts[i]);
					vecIndex.push_back(iVertBase + poly.verts[i + 1]);
				}
			}
		}

		if (vecVertex.empty() || vecIndex.empty()) return nullptr;

		return std::make_shared<Mesh>(vecVertex, vecIndex);
	}
}

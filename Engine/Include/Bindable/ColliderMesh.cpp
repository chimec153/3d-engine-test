#include "ColliderMesh.h"
#include "../Collision/Collision.h"
#include "ColliderLine.h"

Engine::ColliderMesh::ColliderMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex) :
    Collider()
    , m_pInfo()
{
    SetInfo(vecPoint, vecIndex);
    SetComponentType(COMPONENT_TYPE::COLLIDER_MESH);
    SetColliderType(Engine::COLLIDER_TYPE::MESH);
}

Engine::ColliderMesh::ColliderMesh() :
    Collider()
{
    SetColliderType(Engine::COLLIDER_TYPE::MESH);
    SetComponentType(COMPONENT_TYPE::COLLIDER_MESH);
}

Engine::ColliderMesh::ColliderMesh(const ColliderMesh& mesh)    :
    Collider(mesh)
    , m_pInfo(mesh.m_pInfo)
{
}

const Engine::PMESHCOLLIDERINFO Engine::ColliderMesh::GetInfo() const
{
    return m_pInfo.get();
}

void Engine::ColliderMesh::SetInfo(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex)
{
    m_pInfo = std::make_shared<MESHCOLLIDERINFO>(vecPoint, vecIndex);
    // Phase E7 — debug Drawable removed.
}

void Engine::ColliderMesh::Collision(float fDeltaTime)
{
    __super::Collision(fDeltaTime);
}

bool Engine::ColliderMesh::Collision(Collider* pDest, float fDeltaTime)
{
    switch (pDest->GetColliderType())
    {
    case Engine::COLLIDER_TYPE::LINE:
        switch (GetColliderType())
        {
        case Engine::COLLIDER_TYPE::MESH:
            return Collision::CollisionLineToMesh(static_cast<ColliderLine*>(pDest), this);
        case Engine::COLLIDER_TYPE::TERRAIN:
            return Collision::CollisionLineToTerrain(static_cast<ColliderLine*>(pDest), this);
        }
    case Engine::COLLIDER_TYPE::SPHERE:
        break;
    case Engine::COLLIDER_TYPE::MESH:
        break;
    }

    return false;
}

void Engine::ColliderMesh::PreDraw(float fDeltaTime)
{
    // Phase E7 — debug visualization removed.
    __super::PreDraw(fDeltaTime);
}

std::shared_ptr<Engine::Component> Engine::ColliderMesh::Clone()
{
    return std::make_shared<ColliderMesh>(*this);
}

void Engine::ColliderMesh::Save(FILE* pFile)
{
    __super::Save(pFile);

    int iPointCount = static_cast<int>(m_pInfo->vecPoint.size());

    fwrite(&iPointCount, 4, 1, pFile);

    if (iPointCount)
    {
        fwrite(&m_pInfo->vecPoint[0], 4, iPointCount, pFile);
    }

    int iIndexCount = static_cast<int>(m_pInfo->vecIndex.size());

    fwrite(&iIndexCount, 4, 1, pFile);

    if (iIndexCount)
    {
        fwrite(&m_pInfo->vecIndex[0], 4, iIndexCount, pFile);
    }
}

void Engine::ColliderMesh::Load(FILE* pFile)
{
    __super::Load(pFile);

    m_pInfo = std::make_shared<MESHCOLLIDERINFO>();

    int iPointCount = 0;

    fread(&iPointCount, 4, 1, pFile);

    if (iPointCount)
    {
        m_pInfo->vecPoint.resize(iPointCount);

        fread(&m_pInfo->vecPoint[0], 4, iPointCount, pFile);
    }

    int iIndexCount = 0;

    fread(&iIndexCount, 4, 1, pFile);

    if (iIndexCount)
    {
        m_pInfo->vecIndex.resize(iIndexCount);

        fread(&m_pInfo->vecIndex[0], 4, iIndexCount, pFile);
    }
}

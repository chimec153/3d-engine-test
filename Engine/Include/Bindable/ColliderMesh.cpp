#include "ColliderMesh.h"
#include "../Collision/Collision.h"
#include "ColliderLine.h"
#include "BindableManager.h"
#include "Mesh.h"

Engine::ColliderMesh::ColliderMesh(const std::vector<float>& vecPoint, const std::vector<int>& vecIndex) :
    Collider()
    , m_pInfo()
{
    SetInfo(vecPoint, vecIndex);

    SetColliderType(Engine::COLLIDER_TYPE::MESH);
}

Engine::ColliderMesh::ColliderMesh() :
    Collider()
{
    SetColliderType(Engine::COLLIDER_TYPE::MESH);
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

#ifdef _DEBUG
	std::vector<VertexStandard> vecVertex;

	for (size_t i = 0; i < vecPoint.size(); i += 3)
	{
		VertexStandard vertex;

		vertex.pos.x = vecPoint[i];
		vertex.pos.y = vecPoint[i + 1];
		vertex.pos.z = vecPoint[i + 2];

		vecVertex.push_back(vertex);
	}

    std::shared_ptr<Drawable> pDebug = std::static_pointer_cast<Drawable>(FindChild("debug"));

    if (!pDebug) {
        pDebug = CreateBindable<Drawable>("debug");

        pDebug->FindAndAddBind<VertexShader>(STANDARD_VS);
        pDebug->FindAndAddBind<PixelShader>("DebugPS");
        pDebug->FindAndAddBind<InputLayout>(STANDARD_INPUT_LAYOUT);
        pDebug->FindAndAddBind<Topology>(STANDARD_TOPOLOGY);
        pDebug->FindAndAddBind<RasterizerState>(WIREFRAME);

        std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

        pDebug->AddChild(pMaterial->Clone());
    }

	std::shared_ptr<Mesh> pMesh = std::static_pointer_cast<Mesh>(pDebug->FindChild("mesh_debug"));

	if (!pMesh) {
		pMesh = pDebug->CreateBindable<Mesh>("mesh_debug", vecVertex, vecIndex, D3D11_USAGE_DYNAMIC);
    }
    else {
        pMesh->SetVertexBuffer(0, &vecVertex[0], static_cast<int>(sizeof(VertexStandard) * vecVertex.size()));
        pMesh->SetIndexBuffer(0, 0, &vecIndex[0], static_cast<int>(4 * vecIndex.size()));
    }


#endif
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

void Engine::ColliderMesh::Bind()
{
#ifdef _DEBUG
    if (GetPrevColliderList().size())
    {
        std::static_pointer_cast<Drawable>(FindChild("debug"))->GetMaterial()->SetDiffuseColor(1.f, 0.f, 0.f, 1.f);
    }
    else
    {
        std::static_pointer_cast<Drawable>(FindChild("debug"))->GetMaterial()->SetDiffuseColor(0.f, 1.f, 0.f, 1.f);
    }
#endif

    __super::Bind();
}

std::shared_ptr<Engine::Bindable> Engine::ColliderMesh::Clone()
{
    return std::make_shared<ColliderMesh>(*this);
}

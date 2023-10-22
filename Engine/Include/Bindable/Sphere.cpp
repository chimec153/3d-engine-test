#include "Sphere.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "InputLayout.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "TransformBuffer.h"
#include "PixelCBuffer.h"
#include "Topology.h"
#include "Material.h"
#include "../Shader/ShaderManager.h"
#include "BindableManager.h"
#include "ColliderSphere.h"
#include "Collider.h"
#include "Mesh.h"
#include "Drawable.h"

namespace Engine
{
	Sphere::Sphere(int iRings, int iSector) :
		Drawable()
		, m_fSpeed(0.f)
		, m_vDir(static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()))
	{
		m_vDir.Normalize();

		FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
		FindAndAddBind<PixelShader>("anisotropic_microfacet PS_NoTexture");
		FindAndAddBind<InputLayout>("Standard");

		std::string name = "Sphere";

		name += std::to_string(iRings);

		name += "_";

		name += std::to_string(iSector);

		std::vector<unsigned int> vecIndex;

		CreateSphereIndex(iRings, iSector, vecIndex);

		std::shared_ptr<Mesh> pMesh = StaticFindBindable<Mesh>(name);

		if (pMesh == nullptr)
		{
			std::vector<VertexStandard> vecVertex;

			CreateSphereVertex<VertexStandard>(iRings, iSector, vecVertex);

			GetSphereVertexTexcoord(iRings, iSector, vecVertex);

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				vecVertex[i].uv.x = atan2(vecVertex[i].normal.x, vecVertex[i].normal.z) / (2.f * PI) + 0.5f;
				vecVertex[i].uv.y = vecVertex[i].normal.y * 0.5f + 0.5f;
			}

			SetTangent(vecVertex, vecIndex);

			SetBoundingSphereInfo(GetBoundingSphere(vecVertex));

			pMesh = StaticCreateBindable<Mesh>(name, vecVertex, vecIndex);
		}

		AddChild(pMesh);

		FindAndAddBind<Topology>("TriangleList");

		std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

		SetMaterial(pMaterial);

		AddChild(pMaterial);
	}

	Sphere::Sphere(const Sphere& sphere) :
		Drawable(sphere)
		, m_fSpeed(3.f)
		, m_vDir(static_cast<float>(rand()), static_cast<float>(rand()), static_cast<float>(rand()))
	{
		m_vDir.Normalize();

		const std::shared_ptr<TransformBuffer>& pTransform = GetTransform();

		if (pTransform != nullptr)
		{
			//pTransform->SetRandomPosAndRotation();
		}
		const std::shared_ptr<Material>& pMaterial = GetMaterial();

		if (pMaterial != nullptr)
		{
			pMaterial->SetRandomColor();
		}

		//int iSize = rand() % 6 + 5;

		//const std::shared_ptr<ColliderSphere>& pCollider = std::static_pointer_cast<ColliderSphere>(FindChild("ColliderSphere"));

		//if (pCollider)
		//{
		//	pCollider->SetCallBack(COLLISION_TYPE::BEGIN, this, &Sphere::CollisionEnter);
		//	pCollider->SetRadius(iSize / 2.f);
		//}

		//GetTransform()->SetScale({ static_cast<float>(iSize),static_cast<float>(iSize), static_cast<float>(iSize) });

	}

	Sphere::~Sphere()
	{
	}

	float Engine::Sphere::GetSpeed() const
	{
		return m_fSpeed;
	}

	const Vector3& Engine::Sphere::GetDir() const
	{
		return m_vDir;
	}

	void Engine::Sphere::SetSpeed(float fSpeed)
	{
		m_fSpeed = fSpeed;
	}

	void Engine::Sphere::SetDir(const Vector3& vDir)
	{
		m_vDir = vDir;
	}

	bool Sphere::Init()
	{
		return __super::Init();
	}

	void Sphere::Update(float fDeltaTime)
	{
		Drawable::CheckRangeAndMove();

		GetTransform()->AddPosition(m_vDir * m_fSpeed * fDeltaTime);

		__super::Update(fDeltaTime);
	}

	void Sphere::Bind()
	{
		__super::Bind();
	}

	std::shared_ptr<Bindable> Sphere::Clone()
	{
		return std::make_shared<Sphere>(*this);
	}

	void Sphere::CollisionEnter(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		switch (pDest->GetColliderType())
		{
		case COLLIDER_TYPE::SPHERE:
		{
			const Vector3& vNormal = (static_cast<Drawable*>(pSrc->GetParent())->GetTransform()->GetPosition() - pSrc->GetCross()).Normalize();

			m_vDir = m_vDir - m_vDir.Dot(vNormal) * 2.f * vNormal;
		}
		break;
		}

	}

	void Sphere::CreateSphereIndex(int iRings, int iSectors, std::vector<unsigned int>& vecIndex)
		// 2 , 4
	{
		for (int i = 0; i < iSectors; ++i)
		{																					//		0
			vecIndex.push_back(0);															//		3	
			vecIndex.push_back((i + 1) % iSectors + 1);										//	4		2
			vecIndex.push_back(i + 1);														//		1
		}																					//		7	
																							//	8		6
		for (int j = 0; j < iRings - 1; ++j)												//		5
		{																					//		9
			for (int i = 0; i < iSectors; ++i)
			{
				vecIndex.push_back(j * iSectors + i + 1);
				vecIndex.push_back((j + 1) * iSectors + (i + 1) % iSectors + 1);
				vecIndex.push_back((j + 1) * iSectors + i + 1);

				vecIndex.push_back(j * iSectors + i + 1);
				vecIndex.push_back(j * iSectors + (i + 1) % iSectors + 1);
				vecIndex.push_back((j + 1) * iSectors + (i + 1) % iSectors + 1);
			}
		}

		for (int i = 0; i < iSectors; ++i)
		{
			vecIndex.push_back((iRings - 1) * iSectors + i + 1);
			vecIndex.push_back((iRings - 1) * iSectors + (i + 1) % iSectors + 1);
			vecIndex.push_back(iRings * iSectors + 1);
		}
	}
}
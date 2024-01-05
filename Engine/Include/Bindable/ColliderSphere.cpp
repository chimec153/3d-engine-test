#include "ColliderSphere.h"
#include "Drawable.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "../Collision/Collision.h"
#ifdef _DEBUG
#include "BindableManager.h"
#include "VertexShader.h"
#include "HullShader.h"
#include "DomainShader.h"
#include "PixelShader.h"
#include "ConstantBuffer.h"
#include "DepthStencilState.h"
#include "Topology.h"
#include "ConstantBuffer.h"
#include "Mesh.h"
#endif
#include "ColliderOBB.h"

namespace Engine
{
	ColliderSphere::ColliderSphere() :
		Collider()
		, m_vOffset()
	{
#ifdef _DEBUG
		std::shared_ptr<Drawable> pDebug = CreateBindable<Drawable>("DebugSphere");
		pDebug->FindAndAddBind<class VertexShader>("PointLightVS");
		pDebug->FindAndAddBind<class HullShader>("PointLightHS");
		pDebug->FindAndAddBind<class DomainShader>("PointLightDS");
		pDebug->FindAndAddBind<class PixelShader>("CollideDebugPS");
		pDebug->FindAndAddBind<class RasterizerState>("WireFrame");
		pDebug->FindAndAddBind<class DepthStencilState>("NoDepth");
		pDebug->FindAndAddBind<class Topology>("1ControlPointPatch");
		pDebug->SetRenderLayer(RENDER_LAYER::ALPHA);

		m_pDebugMaterial = StaticFindBindable<Material>("Material");

		m_pDebugMaterial = std::static_pointer_cast<Material>(m_pDebugMaterial->Clone());

		pDebug->AddChild(m_pDebugMaterial);

		std::shared_ptr<Mesh> pMesh = pDebug->CreateBindable<Mesh>("2point", 1);

		pMesh->SetVertexCount(0, 2);

		pDebug->NotUseShadow();
#endif
		SetColliderType(COLLIDER_TYPE::SPHERE);
	}

	ColliderSphere::ColliderSphere(const ColliderSphere& collider) :
		Collider(collider)
		, m_vOffset(collider.m_vOffset)
		, m_tInfo(collider.m_tInfo)
#ifdef _DEBUG
		, m_pDebugMaterial(collider.m_pDebugMaterial)
#endif
	{
	}

	const SPHERECOLLIDERINFO& ColliderSphere::GetInfo() const
	{
		return m_tInfo;
	}

	void ColliderSphere::SetRadius(float fRadius)
	{
		m_tInfo.fRadius = fRadius;
#ifdef _DEBUG
		std::shared_ptr<Drawable> pSphere = std::static_pointer_cast<Drawable>(FindChild("DebugSphere"));

		if (pSphere)
		{
			std::shared_ptr<Transform> pTransform = pSphere->GetTransform();

			if (pTransform)
			{
				pTransform->SetScale({ m_tInfo.fRadius,m_tInfo.fRadius,m_tInfo.fRadius });
			}
		}
#endif
	}

	void ColliderSphere::SetOffset(const Vector3& vOffset)
	{
		m_vOffset = vOffset;
	}

	void ColliderSphere::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		m_tInfo.vCenter = m_vOffset;

		Bindable* pParent = GetParent();

		if (pParent)
		{
			std::shared_ptr<Transform> pTransform = std::static_pointer_cast<Transform>(pParent->FindChild(BINDABLE_TYPE::TRANSFORM));

			if (pTransform)
			{
				m_tInfo.vCenter += pTransform->GetPosition();
#ifdef _DEBUG
				std::shared_ptr<Drawable> pSphere = std::static_pointer_cast<Drawable>(FindChild("DebugSphere"));

				if (pSphere)
				{
					std::shared_ptr<Transform> pTransform = pSphere->GetTransform();

					if (pTransform)
					{
						pTransform->SetPosition(m_tInfo.vCenter);
					}
				}
#endif
			}
		}
	}

	bool ColliderSphere::Collision(Collider* pDest, float fDeltaTime)
	{
		switch (pDest->GetColliderType())
		{
		case COLLIDER_TYPE::NONE:
			break;
		case COLLIDER_TYPE::LINE:
			return Collision::CollisionLineToSphere(static_cast<ColliderLine*>(pDest), this);
		case COLLIDER_TYPE::SPHERE:
			return Collision::CollisionSphereToSphere(this, static_cast<ColliderSphere*>(pDest), fDeltaTime);
		case COLLIDER_TYPE::OBB:
			return Collision::CollisionOBBToSphere(static_cast<ColliderOBB*>(pDest), this);
		default:
			break;
		}

		return false;
	}

	std::shared_ptr<Bindable> ColliderSphere::Clone()
	{
		return std::make_shared<ColliderSphere>(*this);
	}

	void ColliderSphere::PreDraw(float fDeltaTime)
	{
#ifdef _DEBUG
		std::static_pointer_cast<Drawable>(FindChild("DebugSphere"))->InViewFrustum();
#endif

		__super::PreDraw(fDeltaTime);
	}

	void ColliderSphere::Bind()
	{
#ifdef _DEBUG

		if (GetPrevColliderList().size())
		{
			m_pDebugMaterial->SetDiffuseColor(Red);
		}
		else
		{
			m_pDebugMaterial->SetDiffuseColor(Green);
		}

		__super::Bind();
#endif
	}
	void ColliderSphere::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_vOffset, 12, 1, pFile);
		fwrite(&m_tInfo, sizeof(SPHERECOLLIDERINFO), 1, pFile);
	}
	void ColliderSphere::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_vOffset, 12, 1, pFile);
		fread(&m_tInfo, sizeof(SPHERECOLLIDERINFO), 1, pFile);
	}
}
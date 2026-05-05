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
		// Phase B.4 — debug drawable as direct member; re-parented onto
		// owning Drawable when this Collider is attached.
		auto pDebug = std::make_shared<Drawable>();
		pDebug->SetTag("DebugSphere");
		pDebug->Init();
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

		m_pDebugDrawable = pDebug;
#endif
		SetComponentType(COMPONENT_TYPE::COLLIDER_SPHERE);
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
		if (m_pDebugDrawable)
		{
			std::shared_ptr<Transform> pTransform = m_pDebugDrawable->GetTransform();
			if (pTransform)
				pTransform->SetScale({ m_tInfo.fRadius,m_tInfo.fRadius,m_tInfo.fRadius });
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

		// Phase B.4 — owning Drawable via Component::GetOwner.
		if (Drawable* pOwner = GetOwner())
		{
			std::shared_ptr<Transform> pTransform = pOwner->GetTransform();
			if (pTransform)
			{
				m_tInfo.vCenter += pTransform->GetPosition();
#ifdef _DEBUG
				if (m_pDebugDrawable)
				{
					std::shared_ptr<Transform> pDebugTr = m_pDebugDrawable->GetTransform();
					if (pDebugTr)
						pDebugTr->SetPosition(m_tInfo.vCenter);
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

	std::shared_ptr<Component> ColliderSphere::Clone()
	{
		return std::make_shared<ColliderSphere>(*this);
	}

	void ColliderSphere::PreDraw(float fDeltaTime)
	{
#ifdef _DEBUG
		// Phase B.4 — color update was previously in Bind() (which Component
		// no longer has). Move it into PreDraw so it runs once per frame
		// before the actual render. The owning Drawable's render path picks
		// up m_pDebugDrawable through the re-parented Bindable child list.
		if (m_pDebugDrawable)
		{
			if (m_pDebugMaterial)
			{
				if (GetPrevColliderList().size())
					m_pDebugMaterial->SetDiffuseColor(Red);
				else
					m_pDebugMaterial->SetDiffuseColor(Green);
			}
			m_pDebugDrawable->InViewFrustum();
		}
#endif

		__super::PreDraw(fDeltaTime);
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
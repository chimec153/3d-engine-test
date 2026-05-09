#include "ColliderSphere.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "../Collision/Collision.h"
#include "ColliderOBB.h"

namespace Engine
{
	ColliderSphere::ColliderSphere() :
		Collider()
		, m_vOffset()
	{
		// Phase E7 — debug visualization Drawable removed.
		SetComponentType(COMPONENT_TYPE::COLLIDER_SPHERE);
		SetColliderType(COLLIDER_TYPE::SPHERE);
	}

	ColliderSphere::ColliderSphere(const ColliderSphere& collider) :
		Collider(collider)
		, m_vOffset(collider.m_vOffset)
		, m_tInfo(collider.m_tInfo)
	{
	}

	const SPHERECOLLIDERINFO& ColliderSphere::GetInfo() const
	{
		return m_tInfo;
	}

	void ColliderSphere::SetRadius(float fRadius)
	{
		m_tInfo.fRadius = fRadius;
	}

	void ColliderSphere::SetOffset(const Vector3& vOffset)
	{
		m_vOffset = vOffset;
	}

	void ColliderSphere::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		m_tInfo.vCenter = m_vOffset;

		// Phase E5 — host transform via the host-agnostic helper.
		{
			std::shared_ptr<Transform> pTransform = GetHostTransform();
			if (pTransform)
			{
				m_tInfo.vCenter += pTransform->GetPosition();
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
		// Phase E7 — debug visualization removed.
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
#include "ColliderSphere.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "../Collision/Collision.h"
#include "ColliderOBB.h"
#ifdef _DEBUG
#include "../Render/RenderManager.h"
#include <cmath>
#endif

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
		__super::PreDraw(fDeltaTime);

#ifdef _DEBUG
		// Wireframe overlay — three orthogonal great circles (XY/XZ/YZ
		// planes through the sphere centre). 24 segments per circle reads
		// cleanly without flooding the line buffer.
		auto* pRM = RenderManager::GetInst();
		if (!pRM->IsDebugDrawColliders()) return;

		constexpr int   kSegs = 24;
		const Vector3&  c     = m_tInfo.vCenter;
		const float     r     = m_tInfo.fRadius;
		const float     fStep = 6.2831853f / static_cast<float>(kSegs);

		for (int i = 0; i < kSegs; ++i)
		{
			const float a0 = static_cast<float>(i)     * fStep;
			const float a1 = static_cast<float>(i + 1) * fStep;
			const float s0 = sinf(a0), c0 = cosf(a0);
			const float s1 = sinf(a1), c1 = cosf(a1);

			// XY plane (z = 0)
			pRM->AddDebugLine(
				Vector3(c.x + r * c0, c.y + r * s0, c.z),
				Vector3(c.x + r * c1, c.y + r * s1, c.z));
			// XZ plane (y = 0)
			pRM->AddDebugLine(
				Vector3(c.x + r * c0, c.y, c.z + r * s0),
				Vector3(c.x + r * c1, c.y, c.z + r * s1));
			// YZ plane (x = 0)
			pRM->AddDebugLine(
				Vector3(c.x, c.y + r * c0, c.z + r * s0),
				Vector3(c.x, c.y + r * c1, c.z + r * s1));
		}
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
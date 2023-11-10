#include "ColliderSphere.h"
#include "Drawable.h"
#include "TransformBuffer.h"
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
#endif

namespace Engine
{
	ColliderSphere::ColliderSphere() :
		Collider()
		, m_vOffset()
#ifdef _DEBUG
		, m_pDebugPSConst(StaticFindBindable<class ConstantBuffer<COLOR>>("COLOR"))
#endif
	{
#ifdef _DEBUG
		std::shared_ptr<Drawable> pDebug = CreateBindable<Drawable>("Debug");
		pDebug->AddChild(m_pDebugPSConst);
		pDebug->FindAndAddBind<class VertexShader>("PointLightVS");
		pDebug->FindAndAddBind<class HullShader>("PointLightHS");
		pDebug->FindAndAddBind<class DomainShader>("PointLightDS");
		pDebug->FindAndAddBind<class PixelShader>("CollideDebugPS");
		pDebug->FindAndAddBind<class RasterizerState>("WireFrame");
		pDebug->FindAndAddBind<class DepthStencilState>("NoDepth");
		pDebug->FindAndAddBind<class Topology>("1ControlPointPatch");
#endif
		SetColliderType(COLLIDER_TYPE::SPHERE);
	}

	ColliderSphere::ColliderSphere(const ColliderSphere& collider) :
		Collider(collider)
		, m_vOffset(collider.m_vOffset)
		, m_tInfo(collider.m_tInfo)
#ifdef _DEBUG
		, m_pDebugPSConst(collider.m_pDebugPSConst)
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
		case COLLIDER_TYPE::END:
			break;
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
		std::static_pointer_cast<Drawable>(FindChild("Debug"))->InViewFrustum();
#endif

		__super::PreDraw(fDeltaTime);
	}

	void ColliderSphere::Bind()
	{
#ifdef _DEBUG
		COLOR color = {};

		if (GetPrevColliderList().size())
		{
			color.color[0].m128_f32[0] = 1.f;
			color.color[0].m128_f32[1] = 0.f;
			color.color[0].m128_f32[2] = 0.f;
			color.color[0].m128_f32[3] = 1.f;
		}
		else
		{
			color.color[0].m128_f32[0] = 0.f;
			color.color[0].m128_f32[1] = 1.f;
			color.color[0].m128_f32[2] = 0.f;
			color.color[0].m128_f32[3] = 1.f;
		}

		m_pDebugPSConst->UpdateBuffer(color);

		__super::Bind();
#endif
	}
}
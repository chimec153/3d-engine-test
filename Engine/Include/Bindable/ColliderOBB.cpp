#include "ColliderOBB.h"
#include "../Collision/Collision.h"
#include "ColliderSphere.h"
#include "TransformBuffer.h"
#ifdef _DEBUG
#include "Mesh.h"
#include "RasterizerState.h"
#include "Topology.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "InputLayout.h"
#include "Material.h"
#include "DepthStencilState.h"
#endif

Engine::ColliderOBB::ColliderOBB()	:
	Collider()
{
	SetBindableType(BINDABLE_TYPE::COLLIDER_OBB);
	SetColliderType(COLLIDER_TYPE::OBB);
#ifdef _DEBUG
	std::shared_ptr<Drawable> pDebugBox = CreateBindable<Drawable>("debug_obb");
	pDebugBox->FindAndAddBind<Mesh>("Box");
	pDebugBox->FindAndAddBind<RasterizerState>(WIREFRAME);
	pDebugBox->FindAndAddBind<Topology>("TriangleList");
	pDebugBox->FindAndAddBind<VertexShader>("anisotropic_microfacet VSNoSkin");
	pDebugBox->FindAndAddBind<PixelShader>("DebugAlphaPS");
	pDebugBox->FindAndAddBind<InputLayout>("Standard");
	pDebugBox->FindAndAddBind<DepthStencilState>("NoDepth");

	m_pDebugTransform = pDebugBox->GetTransform();

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	m_pDebugMaterial = std::static_pointer_cast<Material>(pMaterial->Clone());

	pDebugBox->AddChild(m_pDebugMaterial);

	pDebugBox->NotUseShadow();

	pDebugBox->SetRenderLayer(RENDER_LAYER::ALPHA);
#endif
}

Engine::ColliderOBB::ColliderOBB(const ColliderOBB& collider)	:
	Collider(collider)
	, m_tInfo(collider.m_tInfo)
	, m_vOffset(collider.m_vOffset)
	, m_vScaleOffset(collider.m_vScaleOffset)
	, m_vAxisOffset(collider.m_vAxisOffset)
{
#ifdef _DEBUG
	std::shared_ptr<Drawable> pDebugBox = std::static_pointer_cast<Drawable>(FindChild("debug_obb"));

	if (pDebugBox)
	{
		m_pDebugTransform = pDebugBox->GetTransform();

		m_pDebugMaterial = pDebugBox->GetMaterial();
	}
#endif
}

void Engine::ColliderOBB::SetAxis(AXIS_TYPE eType, const Vector3& vAxis)
{
	assert(static_cast<int>(eType) >= 0 && eType < AXIS_TYPE::END);
	m_tInfo.vAxis[static_cast<int>(eType)] = vAxis;
}

void Engine::ColliderOBB::SetCenter(const Vector3& vCenter)
{
	m_tInfo.vCenter = vCenter;
}

void Engine::ColliderOBB::SetOffset(const Vector3& vOffset)
{
	m_vOffset = vOffset;
}

void Engine::ColliderOBB::SetScaleOffset(const Vector3& vScale)
{
	m_vScaleOffset = vScale;
}

void Engine::ColliderOBB::SetAxisOffset(const Vector3& vOffset)
{
	m_vAxisOffset = vOffset;
}

const Engine::OBBINFO& Engine::ColliderOBB::GetInfo() const
{
	return m_tInfo;
}

void Engine::ColliderOBB::Update(float fDeltaTime)
{
	__super::Update(fDeltaTime);

	Bindable* pParent = GetParent();

	if(pParent)
	{
		std::shared_ptr<Transform> pTransform = static_cast<Drawable*>(pParent)->GetTransform();

		if (pTransform)
		{
			m_tInfo.vCenter = m_vOffset + pTransform->GetTransformMatrix().v[3];

			for (int i = 0; i < 3; ++i)
			{
				m_tInfo.vAxis[i] = pTransform->GetAxis(static_cast<AXIS_TYPE>(i)) * m_vScaleOffset[i];
				m_tInfo.vCenter += pTransform->GetAxis(static_cast<AXIS_TYPE>(i)) * m_vAxisOffset[i];
			}
		}
	}

#ifdef _DEBUG
	if (m_pDebugTransform)
	{
		m_pDebugTransform->SetScale(m_vScaleOffset);
		m_pDebugTransform->SetPosition(m_tInfo.vCenter);
	}
#endif
}

bool Engine::ColliderOBB::Collision(Collider* pCollider, float fDeltaTime)
{
	switch (pCollider->GetColliderType())
	{
	case Engine::COLLIDER_TYPE::NONE:
		break;
	case Engine::COLLIDER_TYPE::LINE:
		break;
	case Engine::COLLIDER_TYPE::SPHERE:
		return Collision::CollisionOBBToSphere(this, static_cast<ColliderSphere*>(pCollider));
	case Engine::COLLIDER_TYPE::MESH:
		break;
	case Engine::COLLIDER_TYPE::TERRAIN:
		break;
	case Engine::COLLIDER_TYPE::OBB:
		return Collision::CollisionOBBToOBB(this, static_cast<ColliderOBB*>(pCollider));
	case Engine::COLLIDER_TYPE::END:
		break;
	default:
		break;
	}

	return false;
}

void Engine::ColliderOBB::PreDraw(float fDeltaTime)
{
	__super::PreDraw(fDeltaTime);
#ifdef _DEBUG
	if (GetPrevColliderList().size())
	{
		m_pDebugMaterial->SetDiffuseColor(1.f, 0.f, 0.f, 1.f);
	}
	else
	{
		m_pDebugMaterial->SetDiffuseColor(0.f, 1.f, 0.f, 1.f);
	}
#endif
}

std::shared_ptr<Engine::Bindable> Engine::ColliderOBB::Clone()
{
	return std::make_shared<ColliderOBB>(*this);
}

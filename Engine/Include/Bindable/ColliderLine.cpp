#include "ColliderLine.h"
#include "../Collision/Collision.h"
#include "ColliderSphere.h"
#include "Drawable.h"
#include "Transform.h"
#include "ColliderMesh.h"
#include "ColliderOBB.h"
#ifdef _DEBUG
#include "BindableManager.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "InputLayout.h"
#include "Topology.h"
#include "Material.h"
#include "Mesh.h"
#endif

namespace Engine
{
	ColliderLine::ColliderLine() :
		Collider()
		, m_vStartOffset()
		, m_vEndOffset(0.f, 0.f, 1.f)
	{
#ifdef _DEBUG
		// Phase B.4 — Collider is a Component now, can't use CreateBindable
		// (a Bindable method). Construct the debug drawable directly; it
		// gets re-parented onto the owning Drawable when this Collider is
		// attached via Drawable::AddChild(Component).
		auto pDebug = std::make_shared<Drawable>();
		pDebug->SetTag("Debug");
		pDebug->Init();
		pDebug->FindAndAddBind<VertexShader>("anisotropic_microfacet VS");
		pDebug->FindAndAddBind<PixelShader>("DebugPS");
		pDebug->FindAndAddBind<Mesh>("Line");
		pDebug->FindAndAddBind<Topology>("LineList");
		pDebug->FindAndAddBind<InputLayout>("TPNT");

		std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");
		pDebug->AddChild(pMaterial->Clone());
		pDebug->NotUseShadow();

		m_pDebugDrawable = pDebug;
#endif
		SetComponentType(COMPONENT_TYPE::COLLIDER_LINE);
		SetColliderType(COLLIDER_TYPE::LINE);
	}

	ColliderLine::ColliderLine(const ColliderLine& line)	:
		Collider(line)
		, m_vStartOffset(line.m_vStartOffset)
		, m_vEndOffset(line.m_vEndOffset)
		, m_tInfo(line.m_tInfo)
	{
	}

	const LINECOLLIDERINFO& ColliderLine::GetInfo() const
	{
		return m_tInfo;
	}

	void ColliderLine::SetStartOffset(const Vector3& vOffset)
	{
		m_vStartOffset = vOffset;
	}

	void ColliderLine::SetEndOffset(const Vector3& vOffset)
	{
		m_vEndOffset = vOffset;
	}

	void ColliderLine::SetStartOffset(float x, float y, float z)
	{
		SetStartOffset({ x,y,z });
	}

	void ColliderLine::SetEndOffset(float x, float y, float z)
	{
		SetEndOffset({ x,y,z });
	}

	void ColliderLine::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		float fLength = (m_vEndOffset - m_vStartOffset).Length();

		if (fLength)
		{
			m_tInfo.vDir = (m_vEndOffset - m_vStartOffset) / fLength;

#ifdef _DEBUG
			// Phase B.4 — debug drawable is now a direct member, not a
			// child. Use the cached pointer instead of FindChild lookup.
			if (m_pDebugDrawable)
			{
				m_pDebugDrawable->GetTransform()->SetRelativePosition(m_tInfo.vStart);
				Vector3 vUp = Vector3(0.f, 0.5f, 0.5f).Normalize();
				m_pDebugDrawable->GetTransform()->SetAxis(AXIS_TYPE::Z, m_tInfo.vDir, vUp);
			}
#endif
		}
		else
		{
			m_tInfo.vDir = 0.f;
		}

		// Phase E5 — host transform via the host-agnostic helper.
		{
			std::shared_ptr<Transform> pTransform = GetHostTransform();
			if (pTransform)
			{
				m_tInfo.vStart = pTransform->GetPosition();
			}
		}
	}

	bool ColliderLine::Collision(Collider* pDest, float fDeltaTime)
	{
		switch (pDest->GetColliderType())
		{
		case COLLIDER_TYPE::NONE:
			break;
		case COLLIDER_TYPE::LINE:
			break;
		case COLLIDER_TYPE::SPHERE:
			return Collision::CollisionLineToSphere(this, static_cast<ColliderSphere*>(pDest));
		case COLLIDER_TYPE::MESH:
			return Collision::CollisionLineToMesh(this, static_cast<ColliderMesh*>(pDest));
		case COLLIDER_TYPE::TERRAIN:
			return Collision::CollisionLineToTerrain(this, static_cast<ColliderMesh*>(pDest));
		case COLLIDER_TYPE::OBB:
			return Collision::CollisionOBBToLine(static_cast<ColliderOBB*>(pDest), this);
		}

		return false;
	}

	void ColliderLine::PreDraw(float fDeltaTime)
	{
#ifdef _DEBUG
		// Phase B.4 — debug drawable is now a direct member; the owning
		// Drawable's render path picks it up because it's been re-parented
		// onto the owner's m_ChildList<Bindable>. We just update the
		// per-frame collision-state color.
		if (m_pDebugDrawable && m_pDebugDrawable->GetMaterial())
		{
			if (GetPrevColliderList().size())
				m_pDebugDrawable->GetMaterial()->SetDiffuseColor(1.f, 0.f, 0.f, 1.f);
			else
				m_pDebugDrawable->GetMaterial()->SetDiffuseColor(0.f, 1.f, 0.f, 1.f);

			m_pDebugDrawable->InViewFrustum();
		}

		__super::PreDraw(fDeltaTime);
#endif
	}
	std::shared_ptr<Component> ColliderLine::Clone()
	{
		return std::make_shared<ColliderLine>(*this);
	}
	void ColliderLine::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_vStartOffset, 12, 1, pFile);
		fwrite(&m_vEndOffset, 12, 1, pFile);
	}
	void ColliderLine::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_vStartOffset, 12, 1, pFile);
		fread(&m_vEndOffset, 12, 1, pFile);
	}
}
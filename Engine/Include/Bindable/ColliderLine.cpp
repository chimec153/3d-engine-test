#include "ColliderLine.h"
#include "../Collision/Collision.h"
#include "ColliderSphere.h"
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
#include "../Render/RenderManager.h"
#endif

namespace Engine
{
	ColliderLine::ColliderLine() :
		Collider()
		, m_vStartOffset()
		, m_vEndOffset(0.f, 0.f, 1.f)
	{
		// Phase E7 — debug visualization Drawable removed (no live host
		// to re-parent it onto post-Phase E5).
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
		__super::PreDraw(fDeltaTime);

#ifdef _DEBUG
		// Wireframe overlay — single segment from vStart along vDir for
		// the original (m_vEndOffset - m_vStartOffset) length. vDir was
		// normalised in Update so the length isn't in m_tInfo; recompute
		// it from the source offsets here.
		auto* pRM = RenderManager::GetInst();
		if (!pRM->IsDebugDrawColliders()) return;

		const float fLength = (m_vEndOffset - m_vStartOffset).Length();
		if (fLength <= 0.f) return;

		const Vector3& p0 = m_tInfo.vStart;
		const Vector3  p1 = p0 + m_tInfo.vDir * fLength;
		pRM->AddDebugLine(p0, p1);
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
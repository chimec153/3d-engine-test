#include "ColliderOBB.h"
#include "../Collision/Collision.h"
#include "ColliderSphere.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "Camera.h"
#ifdef _DEBUG
#include "../Render/RenderManager.h"
#endif

Engine::ColliderOBB::ColliderOBB()	:
	Collider()
{
	SetComponentType(COMPONENT_TYPE::COLLIDER_OBB);
	SetColliderType(COLLIDER_TYPE::OBB);
	// Phase E7 — debug visualization Drawable removed.
}

Engine::ColliderOBB::ColliderOBB(const ColliderOBB& collider)	:
	Collider(collider)
	, m_tInfo(collider.m_tInfo)
	, m_vOffset(collider.m_vOffset)
	, m_vScaleOffset(collider.m_vScaleOffset)
	, m_vAxisOffset(collider.m_vAxisOffset)
{
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
}

bool Engine::ColliderOBB::Collision(Collider* pCollider, float fDeltaTime)
{
	switch (pCollider->GetColliderType())
	{
	case Engine::COLLIDER_TYPE::NONE:
		break;
	case Engine::COLLIDER_TYPE::LINE:
		return Collision::CollisionOBBToLine(this, static_cast<ColliderLine*>(pCollider));
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

void Engine::ColliderOBB::PostUpdate(float fDeltaTime)
{
	__super::PostUpdate(fDeltaTime);

	// Phase E5 — host transform via the host-agnostic helper (works for
	// both Drawable and GameObject hosts).
	{
		std::shared_ptr<Transform> pTransform = GetHostTransform();

		if (pTransform)
		{
			if (pTransform->GetCameraType() == CAMERA_TYPE::UI)
			{
				std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

				std::shared_ptr<Camera> pUICamera = Graphics::GetInst()->GetCamera(CAMERA_TYPE::UI);

				if (pCamera)
				{
					std::shared_ptr<Transform> pCameraTransform = pCamera->GetTransform();

					Vector3 vPos = pTransform->GetPosition() + m_vScaleOffset / 2.f + m_vOffset;

					Vector3 vRightPos = vPos;

					Vector3 vUpPos = vPos;

					vRightPos.x += m_vScaleOffset.x;

					vUpPos.y += m_vScaleOffset.y;

					Vector3 vClipPos = pUICamera->ScreenPosToClipPos({ vPos.x, vPos.y });

					Vector3 vClipRightPos = pUICamera->ScreenPosToClipPos({ vRightPos.x, vRightPos.y });

					Vector3 vClipUpPos = pUICamera->ScreenPosToClipPos({ vUpPos.x, vUpPos.y });

					Vector3 vWorldPos = pCamera->CameraPosToWorldPos({ vClipPos.x, vClipPos.y });

					Vector3 vWorldRightPos = pCamera->CameraPosToWorldPos({ vClipRightPos.x, vClipRightPos.y });

					Vector3 vWorldUpPos = pCamera->CameraPosToWorldPos({ vClipUpPos.x, vClipUpPos.y });

					const Vector3& vCamPos = pCameraTransform->GetPosition();

					const Vector3& vAxisZ = (vWorldPos - vCamPos).Normalize();

					const Vector3& vUp = pCameraTransform->GetAxis(Engine::AXIS_TYPE::Y);

					const Vector3& vAxisX = vUp.Cross(vAxisZ).Normalize();

					const Vector3& vAxisY = vAxisZ.Cross(vAxisX);

					Vector3 vScaleOffset[3] = {};

					m_tInfo.vAxis[0] = vAxisX * (vWorldRightPos - vWorldPos).Length();

					m_tInfo.vAxis[1] = vAxisY * (vWorldUpPos - vWorldPos).Length();

					m_tInfo.vAxis[2] = vAxisZ * 0.001f;

					m_tInfo.vCenter = vWorldPos;

					//m_tInfo.vCenter += vAxisX * m_vAxisOffset[0] / Engine::Window::GetInst()->GetWidth() + vAxisY * m_vAxisOffset[1] / Engine::Window::GetInst()->GetHeight();
				}
			}
			else
			{
				m_tInfo.vCenter = m_vOffset + pTransform->GetTransformMatrix().v[3];

				for (int i = 0; i < 3; ++i)
				{
					m_tInfo.vAxis[i] = pTransform->GetAxis(static_cast<AXIS_TYPE>(i)) * m_vScaleOffset[i];
					m_tInfo.vCenter += pTransform->GetAxis(static_cast<AXIS_TYPE>(i)) * m_vAxisOffset[i];
				}
			}
		}
	}

}

void Engine::ColliderOBB::PreDraw(float fDeltaTime)
{
	__super::PreDraw(fDeltaTime);

#ifdef _DEBUG
	// Wireframe overlay — 8 corners + 12 edges of the OBB. m_tInfo.vAxis[i]
	// already encodes both direction and *half-length-times-2* (PostUpdate
	// sets vAxis[i] = unitAxis * m_vScaleOffset[i]), so the half-extent
	// along each axis is |vAxis[i]| / 2, and the corner-offset is
	// vAxis[i] * 0.5 * signCombo.
	auto* pRM = RenderManager::GetInst();
	if (!pRM->IsDebugDrawColliders()) return;

	const Vector3& c  = m_tInfo.vCenter;
	const Vector3  ax = m_tInfo.vAxis[0] * 0.5f;
	const Vector3  ay = m_tInfo.vAxis[1] * 0.5f;
	const Vector3  az = m_tInfo.vAxis[2] * 0.5f;

	// 8 corners indexed by (sx, sy, sz) ∈ {-1, +1}^3.
	auto corner = [&](int sx, int sy, int sz)
	{
		return c + ax * static_cast<float>(sx)
		         + ay * static_cast<float>(sy)
		         + az * static_cast<float>(sz);
	};

	const Vector3 c000 = corner(-1, -1, -1);
	const Vector3 c100 = corner(+1, -1, -1);
	const Vector3 c010 = corner(-1, +1, -1);
	const Vector3 c110 = corner(+1, +1, -1);
	const Vector3 c001 = corner(-1, -1, +1);
	const Vector3 c101 = corner(+1, -1, +1);
	const Vector3 c011 = corner(-1, +1, +1);
	const Vector3 c111 = corner(+1, +1, +1);

	// Bottom rectangle (sz = -1)
	pRM->AddDebugLine(c000, c100);
	pRM->AddDebugLine(c100, c110);
	pRM->AddDebugLine(c110, c010);
	pRM->AddDebugLine(c010, c000);
	// Top rectangle (sz = +1)
	pRM->AddDebugLine(c001, c101);
	pRM->AddDebugLine(c101, c111);
	pRM->AddDebugLine(c111, c011);
	pRM->AddDebugLine(c011, c001);
	// Vertical edges
	pRM->AddDebugLine(c000, c001);
	pRM->AddDebugLine(c100, c101);
	pRM->AddDebugLine(c110, c111);
	pRM->AddDebugLine(c010, c011);
#endif
}

std::shared_ptr<Engine::Component> Engine::ColliderOBB::Clone()
{
	return std::make_shared<ColliderOBB>(*this);
}

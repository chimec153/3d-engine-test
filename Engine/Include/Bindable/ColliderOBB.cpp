#include "ColliderOBB.h"
#include "../Collision/Collision.h"
#include "ColliderSphere.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "Camera.h"

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
	// Phase E7 — debug visualization removed.
	__super::PreDraw(fDeltaTime);
}

std::shared_ptr<Engine::Component> Engine::ColliderOBB::Clone()
{
	return std::make_shared<ColliderOBB>(*this);
}

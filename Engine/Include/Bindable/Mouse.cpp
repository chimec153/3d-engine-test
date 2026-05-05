#include "Mouse.h"
#include "../Core/Graphics.h"
#include "../Core/Window.h"
#include "Camera.h"
#include "Transform.h"
#include "ColliderLine.h"
#include "../Input/Input.h"
#ifdef _DEBUG
#include "../Render/RenderManager.h"
#endif

namespace Engine
{
	Mouse::Mouse() :
		Component()
		, m_pLineCollider()
	{
		SetComponentType(COMPONENT_TYPE::MOUSE);
	}

	bool Mouse::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		m_pTransform = std::make_shared<Transform>();
		AddChild(m_pTransform);

		m_pLineCollider = CreateComponent<class ColliderLine>("MouseLine");

		if (!m_pLineCollider)
		{
			return false;
		}

		m_pLineCollider->SetChannel(static_cast<COLLISION_CHANNEL>(static_cast<int>(COLLISION_CHANNEL::NORMAL) | static_cast<int>(COLLISION_CHANNEL::UI)));

		return true;
	}

	std::shared_ptr<Component> Mouse::Clone()
	{
		return std::make_shared<Mouse>();
	}

	void Mouse::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

		if (pCamera)
		{
			const std::shared_ptr<Transform>& pTransform = pCamera->GetTransform();
			const Vector3& vPos = pTransform->GetPosition();

			const Vector3& vAxis = pTransform->GetAxis(AXIS_TYPE::Z);

			float iX = CInput::GetInst()->GetMouseX() / static_cast<float>(Window::GetInst()->GetWidth()) * 2.f - 1.f;

			float iY = 1.f - CInput::GetInst()->GetMouseY() / static_cast<float>(Window::GetInst()->GetHeight()) * 2.f;

			const Vector3& vWorldPos = pCamera->CameraPosToWorldPos({ iX, iY });

			m_pTransform->SetPosition(vWorldPos);

			m_pLineCollider->SetStartOffset(vWorldPos);

			m_pLineCollider->SetEndOffset(vWorldPos + vWorldPos - vPos);
		}
	}

	void Mouse::Save(FILE* pFile)
	{
		__super::Save(pFile);
	}

	void Mouse::Load(FILE* pFile)
	{
		__super::Load(pFile);

		// Phase B.4/B.6 — Collider is a Component, Mouse is a Component;
		// look up via Component::FindChild (Mouse's child list).
		m_pLineCollider = std::static_pointer_cast<ColliderLine>(FindChild("MouseLine"));
	}
}
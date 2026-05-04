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
		Drawable()
		, m_pLineCollider()
	{
		SetBindableType(BINDABLE_TYPE::MOUSE);
	}

	bool Mouse::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		m_pLineCollider = CreateBindable<class ColliderLine>("MouseLine");

		if (!m_pLineCollider)
		{
			return false;
		}

		m_pLineCollider->SetChannel(static_cast<COLLISION_CHANNEL>(static_cast<int>(COLLISION_CHANNEL::NORMAL) | static_cast<int>(COLLISION_CHANNEL::UI)));

		return true;
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

			GetTransform()->SetPosition(vWorldPos);

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

		m_pLineCollider = std::static_pointer_cast<ColliderLine>(FindChild("MouseLine"));
	}
}
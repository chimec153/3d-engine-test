#include "Mouse.h"
#include "../Core/Graphics.h"
#include "../Core/Window.h"
#include "Camera.h"
#include "TransformBuffer.h"
#include "ColliderLine.h"
#include "../Input/Input.h"
#ifdef _DEBUG
#include "../Render/RenderManager.h"
#endif

namespace Engine
{
	Mouse::Mouse() :
		Drawable()
		, m_pLineCollider(CreateBindable<class ColliderLine>("MouseLine"))
	{
	}

	void Mouse::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		const std::shared_ptr<Transform>& pTransform = Graphics::GetInst()->GetCamera()->GetTransform();

		const Vector3& vPos = pTransform->GetPosition();

		const Vector3& vAxis = pTransform->GetAxis(AXIS_TYPE::Z);

		float iX = CInput::GetInst()->GetMouseX() / static_cast<float>(Window::GetInst()->GetWidth()) * 2.f - 1.f;

		float iY = 1.f - CInput::GetInst()->GetMouseY() / static_cast<float>(Window::GetInst()->GetHeight()) * 2.f;

		const Matrix& matProject = Graphics::GetInst()->GetProjectMatrix();

		Vector3 vViewPos = {};

		vViewPos.z = matProject[3][2] / -matProject[2][2];

		vViewPos.x = iX / matProject[0][0] * vViewPos.z;
		vViewPos.y = iY / matProject[1][1] * vViewPos.z;

		const Vector3& vWorldPos = pTransform->GetRotationTranslationMatrix().TransformCoord(vViewPos);

		GetTransform()->SetPosition(vWorldPos);

		m_pLineCollider->SetStartOffset(vWorldPos);

		m_pLineCollider->SetEndOffset(vWorldPos + vWorldPos - vPos);
	}
}
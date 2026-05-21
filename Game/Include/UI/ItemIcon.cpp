#include "ItemIcon.h"
#include "Bindable/ColliderOBB.h"
#include "Bindable/Transform.h"
#include "Input/Input.h"
#include "Inventory.h"

namespace Client
{
	ItemIcon::ItemIcon(const std::string& strTexture)	:
		Engine::Image(strTexture)
		, m_bDrag(false)
	{
	}

	void ItemIcon::SetOwner(std::weak_ptr<Engine::UIControl> pOwner)
	{
		m_pOwner = pOwner;
	}

	bool ItemIcon::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		std::shared_ptr<Engine::ColliderOBB> pItemCollider = CreateComponent<Engine::ColliderOBB>("itemcollider");

		if (!pItemCollider)
		{
			return false;
		}

		pItemCollider->SetChannel(Engine::COLLISION_CHANNEL::UI);

		pItemCollider->SetScaleOffset({ 32.f, 32.f, 1.f });

		pItemCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, this, &ItemIcon::CollisionStay);

		std::shared_ptr<Engine::Transform> pItemIconTransform = GetTransform();

		if (!pItemIconTransform)
		{
			return false;
		}

		pItemIconTransform->SetCameraType(Engine::CAMERA_TYPE::UI);

		pItemIconTransform->SetScale(32.f, 32.f, 1.f);

		return true;
	}

	void ItemIcon::Update(float fDeltaTime)
	{
		if (m_bDrag)
		{
			if (Engine::CInput::GetInst()->IsMouseButtonUp(Engine::CInput::MOUSE_TYPE::LEFT))
			{
				m_bDrag = false;

				std::shared_ptr<Engine::UIControl> pOwner = m_pOwner.lock();

				if (typeid(*pOwner.get()) == typeid(Inventory))
				{
					std::static_pointer_cast<Inventory>(pOwner)->DropItem(Engine::CInput::GetInst()->GetMouseX(), Engine::Window::GetInst()->GetHeight() - Engine::CInput::GetInst()->GetMouseY(), this);
				}
			}

			if (Engine::CInput::GetInst()->IsMouseButtonPress(Engine::CInput::MOUSE_TYPE::LEFT))
			{
				const Engine::Vector3& vScale = GetTransform()->GetScale();

				GetTransform()->SetPosition(Engine::CInput::GetInst()->GetMouseX() - vScale.x / 2.f, Engine::Window::GetInst()->GetHeight() - Engine::CInput::GetInst()->GetMouseY() - vScale.y / 2.f, 0.f);
			}
		}
	}

	void ItemIcon::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
		if (!m_bDrag)
		{
			if (Engine::CInput::GetInst()->IsMouseButtonDown(Engine::CInput::MOUSE_TYPE::LEFT) &&
				pDest->GetTag() == "MouseLine")
			{
				m_bDrag = true;
			}
		}
	}
}
#include "Bullet.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Mesh.h"

namespace Client
{
	Bullet::Bullet()	:
		m_fSpeed(0.5f)
	{
	}

	bool Bullet::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		Load(TEXT("Bullet\\Bullet.obj"));

		GetTransform()->SetScale(0.001f, 0.001f, 0.001f);

		std::shared_ptr<Engine::ColliderSphere> pSphere = CreateBindable<Engine::ColliderSphere>("bullet_body");

		pSphere->SetRadius(0.001f);
		 
		return true;
	}

	void Bullet::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		GetTransform()->AddPosition(GetTransform()->GetAxis(Engine::AXIS_TYPE::Y) * fDeltaTime * m_fSpeed);
	}

}
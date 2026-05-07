#include "Bullet.h"
#include "Bindable/Transform.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/Texture.h"
#include "Bindable/Drawable.h"
#include "Component/MeshRendererComponent.h"

namespace Client
{
	Bullet::Bullet()	:
		m_fSpeed(0.5f)
	{
	}

	bool Bullet::Init()
	{
		if (!__super::Init())
			return false;

		// Phase E5 — assemble the bullet entity from Components.
		m_pTransform = AddComponent<Engine::Transform>("transform");
		m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

		// Load .obj and install Mesh / Material / Texture / VS / PS / IL /
		// Topology / etc. into the MeshRenderer in one shot. Replaces the
		// old Drawable::Load(...) call that populated the Drawable's
		// Bindable child list (the children Drawable::Bind iterated for
		// rendering) — same pipeline state, just routed into a Component
		// instead of a Drawable subclass.
		Engine::Drawable::LoadIntoMeshRenderer(TEXT("Bullet\\Bullet.obj"), MESH_PATH, m_pMeshRenderer);

		if (m_pTransform)
			m_pTransform->SetScale(0.001f, 0.001f, 0.001f);

		m_pCollider = AddComponent<Engine::ColliderSphere>("bullet_body");
		if (m_pCollider)
			m_pCollider->SetRadius(0.001f);

		return true;
	}

	void Bullet::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (m_pTransform)
			m_pTransform->AddPosition(m_pTransform->GetAxis(Engine::AXIS_TYPE::Y) * fDeltaTime * m_fSpeed);
	}
}

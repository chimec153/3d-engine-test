#include "Bullet.h"
#include "Core/ObjectFactory.h"
REGISTER_GAMEOBJECT(Client::Bullet, Bullet)
#include "Bindable/Transform.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/Texture.h"
#include "Bindable/Sphere.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Particle.h"
#include "Bindable/BindableManager.h"
#include "Component/MeshRendererComponent.h"

namespace Client
{
	Bullet::Bullet()	:
		m_fSpeed(8.f)
	{
	}

	bool Bullet::Init()
	{
		if (!__super::Init())
			return false;

		// Phase E5 — assemble the bullet entity from Components.
		m_pTransform = AddComponent<Engine::Transform>("transform");
		m_pMeshRenderer = AddComponent<Engine::MeshRendererComponent>("mesh_renderer");

		// /Game/Mesh/Bullet/Bullet.obj doesn't ship — build a unit sphere
		// procedurally via the engine's Sphere helpers and feed it through
		// MeshRendererComponent the same way VoxelChunk does for its mesh.
		std::vector<Engine::VertexStandard> verts;
		std::vector<unsigned int>           inds;
		Engine::Sphere::CreateSphereVertex<Engine::VertexStandard>(8, 16, verts);
		Engine::Sphere::GetSphereVertexTexcoord<Engine::VertexStandard>(8, 16, verts);
		Engine::Sphere::CreateSphereIndex(8, 16, inds);
		auto pMesh = std::make_shared<Engine::Mesh>(verts, inds);

		if (m_pMeshRenderer)
		{
			m_pMeshRenderer->SetMesh(pMesh);
			m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_VS));
			m_pMeshRenderer->SetPixelShader (Engine::StaticFindBindable<Engine::PixelShader> (STANDARD_SOLID_PS));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));

			// Clone the default Material so the bullet has its own bright
			// emissive-ish colour (STANDARD_SOLID_PS samples no diffuse
			// texture; the colour comes entirely from the material CB).
			if (auto pSrcMat = Engine::StaticFindBindable<Engine::Material>("Material"))
			{
				auto pMat = std::static_pointer_cast<Engine::Material>(pSrcMat->Clone());
				pMat->SetDiffuseColor (1.f, 0.85f, 0.2f, 1.f);
				pMat->SetSpecularColor(1.f, 1.f,   1.f,  1.f);
				pMat->SetEmissiveColor({ 1.0f, 0.6f, 0.0f, 1.f });
				m_pMeshRenderer->SetMaterial(pMat);
			}
		}

		// Sphere helper generates a unit sphere centred on the origin with
		// radius 0.5. Scale to ~0.15u world so it reads at character size.
		if (m_pTransform)
			m_pTransform->SetScale(0.15f, 0.15f, 0.15f);

		m_pCollider = AddComponent<Engine::ColliderSphere>("bullet_body");
		if (m_pCollider)
			m_pCollider->SetRadius(0.075f);

		// GPU particle trail. Particle owns its own emitter Transform —
		// Bullet::Update mirrors the bullet's position/rotation into it
		// every frame so emitted particles spawn at the bullet's current
		// location and the local-frame velocity (-Y_local = world-backward,
		// since the bullet's RX=-π/2 + RY=yaw puts world-forward on +Y_local)
		// drifts them behind the bullet.
		m_pTrail = AddComponent<Engine::Particle>("trail", 64);
		if (m_pTrail)
		{
			std::shared_ptr<Engine::Texture> pTex =
				Engine::StaticCreateBindable<Engine::Texture>(
					"particletexture", "/Game/Texture/Particle/particle_00.png", TEXTURE_PATH);
			m_pTrail->SetTexture(pTex);
			m_pTrail->SetStartColor({ 1.f, 0.85f, 0.2f, 1.0f });
			m_pTrail->SetEndColor  ({ 1.f, 0.20f, 0.0f, 0.0f });
			m_pTrail->SetStartSize ({ 0.20f, 0.20f });
			m_pTrail->SetEndSize   ({ 0.04f, 0.04f });
			m_pTrail->SetMaxLifeTime(0.5f);
			m_pTrail->SetEmitTime(0.01f);
			m_pTrail->SetAccelaration({ 0.f, 0.f, 0.f });
			// Velocity is interpreted in the emitter Transform's local
			// frame, so -Y here = world-backward once Bullet copies its
			// orientation onto the trail every frame.
			m_pTrail->SetVelocity   ({ 0.f, -0.8f, 0.f });
			m_pTrail->SetMaxVelocity({ 0.f, -0.8f, 0.f });
			m_pTrail->SetMinCreatePosition({ -0.05f, -0.05f, -0.05f });
			m_pTrail->SetMaxCreatePosition({  0.05f,  0.05f,  0.05f });
			m_pTrail->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
		}

		return true;
	}

	void Bullet::Update(float fDeltaTime)
	{
		// Sync the trail's emitter Transform with the bullet's pose
		// before __super::Update — Particle::Update binds the trail's
		// transform CB to project emitted particles into world space.
		// Particle::PostUpdate (engine-side) drives the matrix rebuild
		// later in the frame, so we just keep position/rotation current
		// here.
		if (m_pTrail && m_pTransform)
		{
			auto pTrailTr = m_pTrail->GetTransform();
			if (pTrailTr)
			{
				pTrailTr->SetPosition(m_pTransform->GetPosition());
				pTrailTr->SetRotation(m_pTransform->GetRotation());
			}
		}

		__super::Update(fDeltaTime);

		if (m_pTransform)
			m_pTransform->AddPosition(m_pTransform->GetAxis(Engine::AXIS_TYPE::Y) * fDeltaTime * m_fSpeed);
	}
}

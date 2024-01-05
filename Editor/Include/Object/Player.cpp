#include "Player.h"
#include "Player.h"
#include "Bindable/NavMesh.h"
#include "Bindable/Agent.h"
#include "Core/Window.h"
#include "Bindable/Transform.h"
#include "Bindable/Animation.h"
#include "Animation/Sequence.h"
#include "Animation/Skeleton.h"
#include "Bindable/Mesh.h"
#include "Animation/JointSocket.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/ColliderLine.h"
#include "Bindable/Sphere.h"
#include "Scene/SceneManager.h"
#include "Bindable/BlendState.h"
#include "Bindable/PaperBurn.h"
#include "Scene/Scene.h"

namespace Editor
{
	Player::Player() :
		Drawable()
#ifdef _DEBUG
		, m_pSphere()
#endif
	{
		CreateBindable<Engine::Animation>("anim");

		std::shared_ptr<Engine::Mesh> pMesh = CreateBindable<Engine::Mesh>("Medieval", "Medieval.mesh", MESH_PATH);

		std::shared_ptr<Engine::Sequence> pSequence = std::make_shared<Engine::Sequence>();

		if (pSequence)
		{
			pSequence->LoadFromPath("MedievalCharacterArmature_Idle_Neutral.seq", MESH_PATH);
		}

		std::shared_ptr<Engine::Sequence> pWalkSequence = std::make_shared<Engine::Sequence>();

		if (pWalkSequence)
		{
			pWalkSequence->LoadFromPath("MedievalCharacterArmature_Sword_Slash.seq", MESH_PATH);

			pWalkSequence->UseRootMotion();

			pWalkSequence->Loop();

			for (int i = 0; i < 65; ++i)
			{
				pWalkSequence->SetBlendFactor(i, 1.f);
			}
		}

		std::shared_ptr<Engine::Skeleton> pSkeleton = std::make_shared<Engine::Skeleton>();

		if (pSkeleton)
		{
			pSkeleton->LoadFromPath("Medieval.skel", MESH_PATH);
			GetAnimation()->SetSkeleton(pSkeleton);
		}

		GetAnimation()->AddSequance("idle", pSequence);
		GetAnimation()->AddSequance("attack", pWalkSequence);

		GetAnimation()->SetAdditiveSequence("attack");

		FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSSkin");
		FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
		FindAndAddBind<Engine::InputLayout>("Standard");
		FindAndAddBind<Engine::Topology>("TriangleList");
	}

	Player::Player(const Player& player) :
		Drawable(player)
		, m_pFootLineCollider()
#ifdef _DEBUG
		, m_pSphere()
#endif
	{
		Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();

		std::shared_ptr<Engine::Drawable> pSword = pScene->CreateDrawable<Engine::Drawable>("sword", pScene->FindLayer(DEFAULT_LAYER));

		pSword->Load(TEXT("UltimateRPGItemsBundle\\Sword\\Sword.fbx"), MESH_PATH);
		pSword->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
		pSword->FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
		pSword->FindAndAddBind<Engine::Topology>("TriangleList");
		pSword->FindAndAddBind<Engine::InputLayout>("Standard");

		std::shared_ptr<Engine::Material> pSrcMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

		pSword->AddChild(pSrcMaterial->Clone());

		GetAnimation()->AddSocket(40, pSword);
	}

	bool Player::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

		return true;
	}

	void Player::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (GetAgent() && GetAnimation())
		{
			const Engine::Vector3& vVelocity = GetAgent()->GetAgentVelocity();

			if (vVelocity.Length() < epsilon)
			{
				//ChangeSequence("idle");
			}
			else
			{
				//ChangeSequence("walk");
			}
		}
	}

	void Player::CollisionStay(Engine::Collider* pSrc, Engine::Collider* pDest, float fDeltaTime)
	{
	}

	std::shared_ptr<Engine::Bindable> Player::Clone()
	{
		return std::make_shared<Player>(*this);
	}
}

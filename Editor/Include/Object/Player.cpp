#include "Player.h"
#include "Player.h"
#include "Bindable/NavMesh.h"
#include "Bindable/Agent.h"
#include "Core/Window.h"
#include "Bindable/TransformBuffer.h"
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
			pWalkSequence->LoadFromPath("MedievalCharacterArmature_Walk.seq", MESH_PATH);

			pWalkSequence->UseRootMotion();
		}

		std::shared_ptr<Engine::Skeleton> pSkeleton = std::make_shared<Engine::Skeleton>();

		if (pSkeleton)
		{
			pSkeleton->LoadFromPath("Medieval.skel", MESH_PATH);
			GetAnimation()->SetSkeleton(pSkeleton);
		}

		GetAnimation()->AddSequance("idle", pSequence);
		GetAnimation()->AddSequance("walk", pWalkSequence);

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
	}


	void Player::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		if (GetAgent() && GetAnimation())
		{
			const Engine::Vector3& vVelocity = GetAgent()->GetAgentVelocity();

			if (vVelocity.Length() < epsilon)
			{
				GetAnimation()->ChangeSequence("idle");
			}
			else
			{
				GetAnimation()->ChangeSequence("walk");
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
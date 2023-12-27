#include "GameScene.h"
#include "../Object/Player.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Mesh.h"
#include "Resource/ResourceManager.h"
#include "Bindable/Terrain.h"
#include "Bindable/ColliderMesh.h"
#include "Bindable/NavMesh.h"
#include "Bindable/Agent.h"
#include "../Object/Monster.h"
#include "Bindable/Decal.h"
#include "Bindable/TransformBuffer.h"
#include "Bindable/Topology.h"

Client::GameScene::GameScene()
{
}

bool Client::GameScene::Init()
{
	Engine::StaticCreateBindable<Engine::Mesh>("Medieval", "Medieval.mesh", MESH_PATH);
	Engine::StaticCreateBindable<Engine::Mesh>("Frog", "Frog.mesh", MESH_PATH);

	Engine::ResourceManager::GetInst()->LoadSkeleton("Medieval.skel");
	Engine::ResourceManager::GetInst()->LoadSkeleton("Frog.skel");

	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Death.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Gun_Shoot.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_HitRecieve.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_HitRecieve_2.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun_Pointing.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun_Shoot.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Neutral.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Sword.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Interact.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Kick_Left.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Kick_Right.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Punch_Left.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Punch_Right.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Roll.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Back.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Left.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Right.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Shoot.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Sword_Slash.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Walk.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Wave.seq");

	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Attack.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Death.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Idle.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Jump.seq");

	std::vector<const TCHAR*> vecTexture =
	{
		TEXT("LandScape\\Terrain_Cliff_15_Large.dds"),
		TEXT("LandScape\\BD_Terrain_Cliff05.dds"),
	};

	std::vector<const TCHAR*> vecNormalTexture =
	{
		TEXT("LandScape\\Terrain_Cliff_15_Large_NRM.bmp"),
		TEXT("LandScape\\BD_Terrain_Cliff05_NRM.bmp"),
	};

	std::vector<const TCHAR*> vecSpecularTexture =
	{
		TEXT("LandScape\\Terrain_Cliff_15_Large_SPEC.bmp"),
		TEXT("LandScape\\BD_Terrain_Cliff05_SPEC.bmp"),
	};

	std::vector<const TCHAR*> vecBlendTexture =
	{
		TEXT("LandScape\\baseAlpha.bmp"),
		TEXT("LandScape\\RoadAlpha.bmp"),
	};

	Engine::StaticCreateBindable<Engine::Texture>("TerrainDiffuse", vecTexture, TEXTURE_PATH, 20);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainNormal", vecNormalTexture, TEXTURE_PATH, 21);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainSpecular", vecSpecularTexture, TEXTURE_PATH, 22);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainBlend", vecBlendTexture, TEXTURE_PATH, 24);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainHeight", TEXT("LandScape\\height2.bmp"), TEXTURE_PATH, 16);
	Engine::StaticCreateBindable<Engine::Texture>("SkyBoxTexture", TEXT("TYbvO.jpg"), TEXTURE_PATH, 5);
	Engine::StaticCreateBindable<Engine::Texture>("PaperBurn", TEXT("DefaultBurn.png"), TEXTURE_PATH, 4);

	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodAlbedo", TEXT("Decal\\sgfjdepc_8K_Albedo.tga"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodNormal", TEXT("Decal\\sgfjdepc_8K_Normal.tga"), TEXTURE_PATH, 1);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodOpacity", TEXT("Decal\\sgfjdepc_8K_Opacity.tga"), TEXTURE_PATH, 2);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodRoughness", TEXT("Decal\\sgfjdepc_8K_Roughness.tga"), TEXTURE_PATH, 3);

	Load("Resource\\Scene\\test.scn", ROOT_PATH);

	std::shared_ptr<Engine::Bindable> pTerrain = FindBindable("Terrain");

	std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = std::static_pointer_cast<Engine::ColliderMesh>(pTerrain->FindChild(Engine::BINDABLE_TYPE::COLLIDER_MESH));

	std::shared_ptr<Client::Player> pPlayer = CreateDrawable<Client::Player>("player", FindLayer(DEFAULT_LAYER), 100, 10, 15);

	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pPlayer.get(), &Client::Player::CollisionTerrainStay);

	CreateDrawable<Client::Monster>("frog", FindLayer(DEFAULT_LAYER), 50, 5, 10);

	std::shared_ptr<Engine::Decal> pDecal = CreateProtoType<Engine::Decal>("blooddecal", Engine::SCENE_TYPE::CURRENT);

	pDecal->FindAndAddBind<Engine::PixelShader>(DECAL_PS_PBR);
	pDecal->FindAndAddBind<Engine::Texture>("DecalBloodAlbedo");
	pDecal->FindAndAddBind<Engine::Texture>("DecalBloodNormal");
	pDecal->FindAndAddBind<Engine::Texture>("DecalBloodOpacity");
	pDecal->FindAndAddBind<Engine::Texture>("DecalBloodRoughness");
	pDecal->FindAndAddBind<Engine::Mesh>("Box");
	pDecal->FindAndAddBind<Engine::Topology>("TriangleList");
	pDecal->SetMaxFadeTime(12.f);
	pDecal->StartFade();

	std::shared_ptr<Engine::Material> pDecalMetarial = std::static_pointer_cast<Engine::Material>(pDecal->FindChild(Engine::BINDABLE_TYPE::MATERIAL));

	if (pDecalMetarial)
	{
		pDecalMetarial->SetRoughnessX(1.f);
		pDecalMetarial->SetRoughnessY(1.f);
	}

	std::shared_ptr<Engine::Transform> pDecalTransform = pDecal->GetTransform();

	if (pDecalTransform)
	{
		pDecalTransform->SetPosition(5.f, 31.f, 5.f);

		pDecalTransform->SetScale(2.f, 1.f, 2.f);
	}

	return true;
}
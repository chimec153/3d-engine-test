#include "GameScene.h"
#include "../Object/Player.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Mesh.h"
#include "Resource/ResourceManager.h"
#include "Bindable/Terrain.h"
#include "Bindable/ColliderMesh.h"
#include "Bindable/NavMesh.h"
#include "Bindable/Agent.h"

Client::GameScene::GameScene()
{
}

bool Client::GameScene::Init()
{
	Engine::StaticCreateBindable<Engine::Mesh>("Medieval", "Medieval.mesh", MESH_PATH);

	Engine::ResourceManager::GetInst()->LoadSkeleton("Medieval.skel");

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
	Engine::StaticCreateBindable<Engine::Texture>("SkyBoxTexture", TEXT("gnbRv.jpg"), TEXTURE_PATH, 5);

	Load("Resource\\Scene\\test.scn", ROOT_PATH);

	CreateDrawable<Client::Player>("player", FindLayer(DEFAULT_LAYER));

	return true;
}
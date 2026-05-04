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
#include "Bindable/Transform.h"
#include "Bindable/Topology.h"
#include "UI/Image.h"
#include "UI/Frame.h"
#include "../UI/Inventory.h"
#include "Input/Input.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Bindable/UIRenderer.h"
#include "Sound/Sound.h"
#include "../Object/Tree.h"
#include "Render/RenderManager.h"
#include "Core/PathManager.h"
#include "Bindable/PointLight.h"

Client::GameScene::GameScene()
{
}

bool Client::GameScene::Init()
{
	{
		std::shared_ptr<Engine::Drawable> pPlayer = CreateDrawable<Engine::Drawable>("player", FindLayer(DEFAULT_LAYER));

		pPlayer->Load(TEXT("Walking.fbx"));
	}

	Engine::StaticCreateBindable<Engine::Mesh>("Medieval", "Walking.mesh", MESH_PATH);
	//Engine::StaticCreateBindable<Engine::Mesh>("Frog", "Frog.mesh", MESH_PATH);
	//Engine::StaticCreateBindable<Engine::Mesh>("sword", "Sword.mesh", MESH_PATH);
	//Engine::StaticCreateBindable<Engine::Mesh>("shovel", "Shovel.mesh", MESH_PATH);
	//Engine::StaticCreateBindable<Engine::Mesh>("armor", "Armor_Leather.mesh", MESH_PATH);
	//Engine::StaticCreateBindable<Engine::Mesh>("pistol", "Pistol_5.mesh", MESH_PATH);

	Engine::ResourceManager::GetInst()->LoadSkeleton("Walking.skel");
	//Engine::ResourceManager::GetInst()->LoadSkeleton("Frog.skel");

	Engine::ResourceManager::GetInst()->LoadSequence("Walkingmixamo.com.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Gun_Shoot.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_HitRecieve.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_HitRecieve_2.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun_Pointing.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Gun_Shoot.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Neutral.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Idle_Sword.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Interact.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Kick_Left.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Kick_Right.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Punch_Left.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Punch_Right.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Roll.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Back.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Left.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Right.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Run_Shoot.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Sword_Slash.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Walk.seq");
	//Engine::ResourceManager::GetInst()->LoadSequence("MedievalCharacterArmature_Wave.seq");

	/*Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Attack.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Death.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Idle.seq");
	Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Jump.seq");

	Engine::ResourceManager::GetInst()->CreateSound("leather_inventory", "inventory_sound_effects\\leather_inventory.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
	Engine::ResourceManager::GetInst()->CreateSound("metal-clash", "inventory_sound_effects\\metal-clash.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
	Engine::ResourceManager::GetInst()->CreateSound("sword sound", "melee sounds\\sword sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
	Engine::ResourceManager::GetInst()->CreateSound("step_rock_l", "sfx_step_rock_l.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
	Engine::ResourceManager::GetInst()->CreateSound("step_rock_r", "sfx_step_rock_r.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
	Engine::ResourceManager::GetInst()->CreateSound("melee sound", "melee sounds\\melee sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);*/

	//for (int i = 0; i < 37; ++i)
	//{
	//	char strSound[TEXT_LEN] = {};

	//	sprintf_s(strSound, "hit%02d", i + 1);

	//	std::string strPath = "hits\\";

	//	strPath += strSound;
	//	strPath += ".mp3.flac";

	//	Engine::ResourceManager::GetInst()->CreateSound(strSound, strPath.c_str(), SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
	//}

	//std::shared_ptr<Engine::Sound> pSound = Engine::ResourceManager::GetInst()->CreateSound("TownTheme", "TownTheme.mp3", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, true);

	//pSound->Play();

	std::vector<const TCHAR*> vecTexture =
	{
		TEXT("LandScape\\dirt.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\sand.bmp"),
		TEXT("LandScape\\sand.bmp"),
		TEXT("LandScape\\sand.bmp"),
		TEXT("LandScape\\sand.bmp"),
		TEXT("LandScape\\sand.bmp"),
	};

	std::vector<const TCHAR*> vecNormalTexture =
	{
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
	};

	std::vector<const TCHAR*> vecSpecularTexture =
	{
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
		TEXT("LandScape\\grass.bmp"),
	};

	std::vector<const TCHAR*> vecBlendTexture =
	{
		TEXT("LandScape\\baseAlpha.png"),
		TEXT("LandScape\\baseAlpha.png"),
	};

	Engine::StaticCreateBindable<Engine::Texture>("TerrainDiffuse", vecTexture, TEXTURE_PATH, 20);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainNormal", vecNormalTexture, TEXTURE_PATH, 21);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainSpecular", vecSpecularTexture, TEXTURE_PATH, 22);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainBlend", vecBlendTexture, TEXTURE_PATH, 24);
	Engine::StaticCreateBindable<Engine::Texture>("TerrainHeight", TEXT("LandScape\\height2.png"), TEXTURE_PATH, 16);
	Engine::StaticCreateBindable<Engine::Texture>("SkyBoxTexture", TEXT("TYbvO.png"), TEXTURE_PATH, 5);
	Engine::StaticCreateBindable<Engine::Texture>("PaperBurn", TEXT("DefaultBurn.png"), TEXTURE_PATH, 4);
	Engine::StaticCreateBindable<Engine::Texture>("QuickSlot", TEXT("item.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("frame.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("frame.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("shovel_icon", TEXT("shovel_icon.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("sword_icon", TEXT("sword_icon.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("armor_icon", TEXT("armor_icon.png"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("gun_icon", TEXT("gun_icon.png"), TEXTURE_PATH, 0);

	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodAlbedo", TEXT("Decal\\sgfjdepc_8K_Albedo.tga"), TEXTURE_PATH, 0);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodNormal", TEXT("Decal\\sgfjdepc_8K_Normal.tga"), TEXTURE_PATH, 1);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodOpacity", TEXT("Decal\\sgfjdepc_8K_Opacity.tga"), TEXTURE_PATH, 2);
	Engine::StaticCreateBindable<Engine::Texture>("DecalBloodRoughness", TEXT("Decal\\sgfjdepc_8K_Roughness.tga"), TEXTURE_PATH, 3);

	Load("Resource\\Scene\\test.scn", ROOT_PATH);

	AddLayer(DEFAULT_LAYER);

	if (std::shared_ptr<Engine::Bindable> pTerrain = FindBindable("Terrain"))
	{
		std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = std::static_pointer_cast<Engine::ColliderMesh>(pTerrain->FindChild(Engine::BINDABLE_TYPE::COLLIDER_MESH));
	}

	if (std::shared_ptr<Engine::Camera> pCamera = CreateDrawable<Engine::Camera>("Camera", FindLayer(DEFAULT_LAYER)))
	{
		pCamera->SetProjectType(Engine::Camera::PROJECT_TYPE::PERSPECTIVE);

		Engine::Graphics::GetInst()->SetCamera(pCamera);
	}

	if (auto pLight = CreateDrawable<Engine::PointLight>("Light", FindLayer(DEFAULT_LAYER)))
	{
		pLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);
	}

	std::shared_ptr<Engine::Camera> pUIInventoryCamera = CreateDrawable<Engine::Camera>("InventoryCamera", FindLayer(DEFAULT_LAYER));

	pUIInventoryCamera->SetProjectType(Engine::Camera::PROJECT_TYPE::PERSPECTIVE);

	std::shared_ptr<Client::Player> pPlayer = CreateDrawable<Client::Player>("player", FindLayer(DEFAULT_LAYER), 100, 10, 15);

	pPlayer->AddChild(pUIInventoryCamera);

	std::shared_ptr<Engine::Transform> pInventoryCameraTransform = pUIInventoryCamera->GetTransform();

	if (!pInventoryCameraTransform)
	{
		return false;
	}

	pInventoryCameraTransform->SetRelativePosition(1.f, -0.2f, -20.f);

	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pPlayer.get(), &Client::Player::CollisionTerrainStay);

	CreateDrawable<Client::Monster>("frog", FindLayer(DEFAULT_LAYER), 50, 5, 10);

	/*std::shared_ptr<Engine::Decal> pDecal = CreateProtoType<Engine::Decal>("blooddecal", Engine::SCENE_TYPE::CURRENT);

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

	std::shared_ptr<Engine::Image> pQuickSlot = CreateDrawable<Engine::Image>("QuickSlot", FindLayer(DEFAULT_LAYER), "QuickSlot");

	std::shared_ptr<Engine::Transform> pQuickSlotTransform = pQuickSlot->GetTransform();

	pQuickSlotTransform->SetScale(287.f, 42.f, 1.f);
	pQuickSlotTransform->SetPosition(floor((Engine::Window::GetInst()->GetWidth() - 287.f) / 2.f), 10.f, 0.f);

	std::shared_ptr<Inventory> pInventory = CreateDrawable<Inventory>("Inventory", FindLayer(DEFAULT_LAYER), "frame");

	std::shared_ptr<Engine::Transform> pFrameTransform = pInventory->GetTransform();

	pFrameTransform->SetScale(291.f, 313.f, 1.f);
	pFrameTransform->SetPosition(Engine::Window::GetInst()->GetWidth() / 2 - 145.f, Engine::Window::GetInst()->GetHeight() / 2 - 156.f, 0.f);

	Engine::CInput::GetInst()->AddKey(DIK_I);

	Engine::CInput::GetInst()->CreateAction("Inventory", DIK_I);

	Engine::CInput::GetInst()->AddAction("Inventory", Engine::CInput::KEY_STATE::UP, pInventory.get(), &Inventory::ToggleInventory);

	pInventory->AddItem(1);

	pInventory->AddItem(2);

	pInventory->AddItem(3);

	pInventory->AddItem(4);

	std::shared_ptr<Engine::UIRenderer> pUIRenderer = std::static_pointer_cast<Engine::UIRenderer>(pInventory->FindChild("uirenderer"));

	if (pUIRenderer)
	{
		pUIRenderer->SetCamera(pUIInventoryCamera);

		pUIRenderer->SetTarget(pPlayer);
	}

	std::shared_ptr<Engine::UIRenderer> pUIWeaponRenderer = std::static_pointer_cast<Engine::UIRenderer>(pInventory->FindChild("uirenderer_weapon"));

	if (pUIWeaponRenderer)
	{
		pUIWeaponRenderer->SetCamera(pUIInventoryCamera);
	}

	pPlayer->SetInventory(pInventory);*/

	//std::shared_ptr<Engine::Drawable> pArmor = CreateDrawable<Engine::Drawable>("armor", FindLayer(DEFAULT_LAYER));

	//pArmor->Load(TEXT("UltimateRPGItemsBundle\\ArmorLeather\\Armor_Leather.fbx"));

	//CreateDrawable<Tree>("tree", FindLayer(DEFAULT_LAYER));

	Engine::RenderManager::GetInst()->SetFogColor({0.f, 43.f / 255.f, 152.f / 255.f});
	Engine::RenderManager::GetInst()->SetFogHighlightColor({1.f, 0.f, 0.f});
	Engine::RenderManager::GetInst()->SetFogStartDepth(10.f);
	Engine::RenderManager::GetInst()->SetFogDensity(0.03f);
	Engine::RenderManager::GetInst()->SetFogHeightFallOff(0.001f);

	return true;
}
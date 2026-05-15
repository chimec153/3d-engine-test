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
#include "Bindable/MeshLoader.h"
#include "Component/MeshRendererComponent.h"
#include "Bindable/InputLayout.h"
#include "Bindable/DepthStencilState.h"

namespace Client
{
	GameScene::GameScene()
	{
	}

	bool GameScene::LoadSequences()
	{
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Idle", "Idle.seq");
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Run", "Slow Run.seq");
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Attack", "Standing Melee Attack Downward.seq");
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
		Engine::ResourceManager::GetInst()->LoadSequence("FrogFrogArmature_Frog_Jump.seq");*/
		return true;
	}

	bool GameScene::CreateTexture()
	{
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
		return true;
	}

	bool GameScene::CreateSounds()
	{
		Engine::ResourceManager::GetInst()->CreateSound("leather_inventory", "inventory_sound_effects\\leather_inventory.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
		Engine::ResourceManager::GetInst()->CreateSound("metal-clash", "inventory_sound_effects\\metal-clash.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
		Engine::ResourceManager::GetInst()->CreateSound("sword sound", "melee sounds\\sword sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("step_rock_l", "sfx_step_rock_l.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("step_rock_r", "sfx_step_rock_r.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("melee sound", "melee sounds\\melee sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);

		//for (int i = 0; i < 37; ++i)
		//{
		//	char strSound[TEXT_LEN] = {};

		//	sprintf_s(strSound, "hit%02d", i + 1);

		//	std::string strPath = "hits\\";

		//	strPath += strSound;
		//	strPath += ".mp3.flac";

		//	Engine::ResourceManager::GetInst()->CreateSound(strSound, strPath.c_str(), SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		//}
		return true;
	}

	bool GameScene::CreateMesh()
	{
		Engine::StaticCreateBindable<Engine::Mesh>("Idle", "Idle.mesh", MESH_PATH);
		Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "Idle2.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("Frog", "Frog.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("sword", "Sword.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("shovel", "Shovel.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("armor", "Armor_Leather.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("pistol", "Pistol_5.mesh", MESH_PATH);
		return true;
	}

	bool GameScene::CreateMonster()
	{
		/*auto pMonster = CreateGameObject<Monster>("monster", FindLayer(DEFAULT_LAYER));
		auto pTransform = pMonster->AddComponent<Engine::Transform>("transform");
		auto m_pMeshRenderer = pMonster->AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
		auto m_pAnimation = pMonster->AddComponent<Engine::Animation>("MonsterAnimation");

		if (pTransform)
		{
			pTransform->SetPosition(20.f, 5.f, 10.f);
			pTransform->SetScale(0.01f, 0.01f, 0.01f);
		}

		std::shared_ptr<Engine::Mesh> pMesh = Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "Idle2.mesh");
		if (!pMesh)
		{
			pMesh = Engine::StaticFindBindable<Engine::Mesh>("Idle2");
		}
		if (m_pMeshRenderer)
		{
			m_pMeshRenderer->SetMesh(pMesh);
			m_pMeshRenderer->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>(STANDARD_ANIM_VS));
			m_pMeshRenderer->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>(STANDARD_PS));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
			m_pMeshRenderer->AddBindable(Engine::StaticFindBindable<Engine::DepthStencilState>("OutLineMask"));
			m_pMeshRenderer->SetAnimation(m_pAnimation);
		}*/
		return true;
	}

	bool GameScene::Init()
	{
		// Phase E7 — preload Walking.fbx mesh data into BindableManager via
		// the MeshLoader facade. Drawable's loader bridge has been retired;
		// MeshLoader::Load drives the same parser pipeline without a temp
		// Drawable, and the parsed Bindables register in BindableManager.
		//Engine::MeshLoader::Load(TEXT("Idle.fbx"));
		//Engine::MeshLoader::Load(TEXT("Standing Melee Attack Downward.fbx"));
		//Engine::MeshLoader::Load(TEXT("Slow Run.fbx"));

		CreateMesh();

		Engine::ResourceManager::GetInst()->LoadSkeleton("Idle.skel");
		//Engine::ResourceManager::GetInst()->LoadSkeleton("Frog.skel");

		LoadSequences();

		CreateSounds();

		//std::shared_ptr<Engine::Sound> pSound = Engine::ResourceManager::GetInst()->CreateSound("TownTheme", "TownTheme.mp3", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, true);

		//pSound->Play();

		CreateTexture();

		//Load("Resource\\Scene\\test.scn", ROOT_PATH);

		AddLayer(DEFAULT_LAYER);

		// Phase E5 — Terrain is a GameObject now.
		if (auto pTerrain = CreateGameObject<Engine::Terrain>("Terrain", FindLayer(DEFAULT_LAYER)))
		{
			// Phase E7 — height map keeps Create* (different file path +
			// dynamic-access flag for in-game edits). Diffuse/Normal/Specular
			// were already registered in CreateTexture(); wire them in via
			// the new Set* path so we don't redundantly create-or-find here.
			pTerrain->CreateHeightMap("terrain", TEXT("terraintest.bmp"));
			pTerrain->SetTerrainTexture(Engine::StaticFindBindable<Engine::Texture>("TerrainDiffuse"));
			pTerrain->SetTerrainNormalTexture(Engine::StaticFindBindable<Engine::Texture>("TerrainNormal"));
			pTerrain->SetTerrainSpecularTexture(Engine::StaticFindBindable<Engine::Texture>("TerrainSpecular"));

			std::vector<float> vecPoints;
			std::vector<int> vecTris;

			pTerrain->GetPoints(vecPoints);
			pTerrain->GetTris(vecTris);

			Engine::Vector3 vMin(FLT_MAX, FLT_MAX, FLT_MAX);
			Engine::Vector3 vMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for (size_t i = 0; i + 2 < vecPoints.size(); i += 3)
			{
				float x = vecPoints[i], y = vecPoints[i + 1], z = vecPoints[i + 2];
				vMin.x = std::min(vMin.x, x); vMin.y = std::min(vMin.y, y); vMin.z = std::min(vMin.z, z);
				vMax.x = std::max(vMax.x, x); vMax.y = std::max(vMax.y, y); vMax.z = std::max(vMax.z, z);
			}

			// 기본 config로 빌드 (또는 NavMeshConfig 수정해서 전달)
			Engine::NavMeshConfig cfg;
			cfg.fAgentRadius = 0.5f;   // 게임에 맞춰 조정
			cfg.fAgentHeight = 1.8f;

			auto pNavMesh = Engine::NavMesh::Build(vecPoints, vecTris, vMax, vMin, cfg);

			if (pNavMesh)
			{
				// Terrain GameObject나 별도 NavMesh holder GameObject에 컴포넌트로 부착
				if (auto pNavObj = CreateGameObject("NavMesh", FindLayer(DEFAULT_LAYER)))
				{
					pNavObj->AddComponent(pNavMesh);
				}
				// 이후 Agent 생성/이동에 사용
			}
			{
				auto pDebugMesh = pNavMesh->CreateDebugMesh();

				auto pDebugObj = CreateGameObject("NavDebug", FindLayer(DEFAULT_LAYER));
				pDebugObj->AddComponent<Engine::Transform>("transform");
				auto pMR = pDebugObj->AddComponent<Engine::MeshRendererComponent>("mesh_renderer");
				pMR->SetMesh(pDebugMesh);
				pMR->SetVertexShader(Engine::StaticFindBindable<Engine::VertexShader>("anisotropic_microfacet VSNoSkin"));
				pMR->SetPixelShader(Engine::StaticFindBindable<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal"));
				pMR->AddBindable(Engine::StaticFindBindable<Engine::InputLayout>("Standard"));
				pMR->AddBindable(Engine::StaticFindBindable<Engine::Topology>("TriangleList"));
				pMR->AddBindable(Engine::StaticFindBindable<Engine::RasterizerState>(WIREFRAME));
			}

			// Phase E5 — Terrain is a GameObject; Material is set on the
			// MeshRendererComponent instead of via Drawable's SetMaterial.
			// The MeshRenderer was already given a default material in
			// Terrain::Init; the call here used to overwrite shininess.
			// Re-introduce when MeshRenderer exposes a Material accessor
			// that lets external setup override its existing material
			// without a re-init round trip.

			// Collider lookup via the GameObject's FindComponent.
			std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = std::static_pointer_cast<Engine::ColliderMesh>(pTerrain->FindComponent(Engine::COMPONENT_TYPE::COLLIDER_MESH));
		}

		if (auto pCameraObj = CreateGameObject("Camera", FindLayer(DEFAULT_LAYER)))
		{
			if (std::shared_ptr<Engine::Camera> pCamera = pCameraObj->AddComponent<Engine::Camera>("Camera"))
			{
				pCamera->SetProjectType(Engine::Camera::PROJECT_TYPE::PERSPECTIVE);

				Engine::Graphics::GetInst()->SetCamera(pCamera);
			}
		}

		if (auto pLightObj = CreateGameObject("Light", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pLight = pLightObj->AddComponent<Engine::PointLight>("Light"))
			{
				pLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);
			}
		}

		// Phase E5 — Player is a GameObject now.
		std::shared_ptr<Player> pPlayer = CreateGameObject<Player>("player", FindLayer(DEFAULT_LAYER), 100, 10, 15);

		// InventoryCamera is owned by the Player (previously also registered
		// on Layer's m_ComponentList; that parallel registration was removed
		// when m_ComponentList was deleted).
		std::shared_ptr<Engine::Camera> pUIInventoryCamera = pPlayer->AddComponent<Engine::Camera>("InventoryCamera");

		pUIInventoryCamera->SetProjectType(Engine::Camera::PROJECT_TYPE::PERSPECTIVE);

		std::shared_ptr<Engine::Transform> pInventoryCameraTransform = pUIInventoryCamera->GetTransform();

		if (!pInventoryCameraTransform)
		{
			return false;
		}

		pInventoryCameraTransform->SetRelativePosition(1.f, -0.2f, -20.f);

		//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pPlayer.get(), &Player::CollisionTerrainStay);

		CreateGameObject<Monster>("frog", FindLayer(DEFAULT_LAYER), 50, 5, 10);

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

		Engine::RenderManager::GetInst()->SetFogColor({ 0.f, 43.f / 255.f, 152.f / 255.f });
		Engine::RenderManager::GetInst()->SetFogHighlightColor({ 1.f, 0.f, 0.f });
		Engine::RenderManager::GetInst()->SetFogStartDepth(50.f);   // was 10 — fog 시작 거리 늘림
		Engine::RenderManager::GetInst()->SetFogDensity(0.005f);    // was 0.03 — 농도 약하게
		Engine::RenderManager::GetInst()->SetFogHeightFallOff(0.001f);

		// Phase E7 — RenderV2 retired. The sort-by-state benefit it provided
		// will be reintroduced as additions to the V1 render path; the
		// parallel V2 demos (v2Mesh, TreeV2) and their integration are gone.

		//CreateMonster();
		return true;
	}

	void GameScene::Update(float dt)
	{
		__super::Update(dt);   // Scene::Update auto-advances V2 drawables.
	}

	void GameScene::Draw()
	{
		__super::Draw();
	}
}
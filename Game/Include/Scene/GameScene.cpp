#include "GameScene.h"
#include "Core/ObjectFactory.h"
#include "Voxel/VoxelWorld.h"
REGISTER_SCENE(Client::GameScene, GameScene)
#include "../Object/Player.h"
#include "../Object/WeaponDatabase.h"
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
#include "../UI/HPBar.h"
#include "../UI/XPBar.h"
#include "../UI/LevelUpChoices.h"
#include "../UI/EnemyCountHUD.h"
#include "Input/Input.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Bindable/Texture.h"
#include "Bindable/ConstantBuffer.h"
#include "Matrix.h"
#include "Bindable/UIRenderer.h"
#include "Sound/Sound.h"
#include "../Object/Tree.h"
#include "../Object/Enemy.h"
#include "Render/RenderManager.h"
#include "Core/PathManager.h"
#include "Bindable/PointLight.h"
#include "Bindable/MeshLoader.h"
#include "Component/MeshRendererComponent.h"
#include "Bindable/InputLayout.h"
#include "Bindable/DepthStencilState.h" 
#include "Bindable/Mesh.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Transform.h"
#include "Component/MeshRendererComponent.h"
#include "Bindable/BindableManager.h"
#include <cmath>
#include <algorithm>


namespace Client
{
	GameScene::GameScene()
	{
	}

	// Out-of-line so unique_ptr<VoxelWorld> can destroy a complete type.
	GameScene::~GameScene() = default;

	bool GameScene::LoadSequences()
	{
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Idle",   "/Game/Mesh/Idle.seq");
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Run",    "/Game/Mesh/Slow Run.seq");
		Engine::ResourceManager::GetInst()->LoadSequenceByTag("Attack", "/Game/Mesh/Standing Melee Attack Downward.seq");
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
			TEXT("/Game/Texture/LandScape/dirt.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/sand.bmp"),
			TEXT("/Game/Texture/LandScape/sand.bmp"),
			TEXT("/Game/Texture/LandScape/sand.bmp"),
			TEXT("/Game/Texture/LandScape/sand.bmp"),
			TEXT("/Game/Texture/LandScape/sand.bmp"),
		};

		std::vector<const TCHAR*> vecNormalTexture =
		{
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
		};

		std::vector<const TCHAR*> vecSpecularTexture =
		{
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
			TEXT("/Game/Texture/LandScape/grass.bmp"),
		};

		std::vector<const TCHAR*> vecBlendTexture =
		{
			TEXT("/Game/Texture/LandScape/baseAlpha.png"),
			TEXT("/Game/Texture/LandScape/baseAlpha.png"),
		};

		Engine::StaticCreateBindable<Engine::Texture>("TerrainDiffuse", vecTexture, TEXTURE_PATH, 20);
		Engine::StaticCreateBindable<Engine::Texture>("TerrainNormal", vecNormalTexture, TEXTURE_PATH, 21);
		Engine::StaticCreateBindable<Engine::Texture>("TerrainSpecular", vecSpecularTexture, TEXTURE_PATH, 22);
		Engine::StaticCreateBindable<Engine::Texture>("TerrainBlend", vecBlendTexture, TEXTURE_PATH, 24);
		Engine::StaticCreateBindable<Engine::Texture>("TerrainHeight", TEXT("/Game/Texture/LandScape/height2.png"), TEXTURE_PATH, 16);
		Engine::StaticCreateBindable<Engine::Texture>("SkyBoxTexture", TEXT("/Game/Texture/TYbvO.png"), TEXTURE_PATH, 5);
		Engine::StaticCreateBindable<Engine::Texture>("PaperBurn", TEXT("/Game/Texture/DefaultBurn.png"), TEXTURE_PATH, 4);
		Engine::StaticCreateBindable<Engine::Texture>("QuickSlot", TEXT("/Game/Texture/item.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("/Game/Texture/frame.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("/Game/Texture/frame.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("shovel_icon", TEXT("/Game/Texture/shovel_icon.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("sword_icon", TEXT("/Game/Texture/sword_icon.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("armor_icon", TEXT("/Game/Texture/armor_icon.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("gun_icon", TEXT("/Game/Texture/gun_icon.png"), TEXTURE_PATH, 0);

		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodAlbedo", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Albedo.tga"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodNormal", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Normal.tga"), TEXTURE_PATH, 1);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodOpacity", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Opacity.tga"), TEXTURE_PATH, 2);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodRoughness", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Roughness.tga"), TEXTURE_PATH, 3);
		return true;
	}

	bool GameScene::CreateSounds()
	{
		Engine::ResourceManager::GetInst()->CreateSound("leather_inventory", "/Game/Sound/inventory_sound_effects/leather_inventory.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
		Engine::ResourceManager::GetInst()->CreateSound("metal-clash", "/Game/Sound/inventory_sound_effects/metal-clash.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);
		Engine::ResourceManager::GetInst()->CreateSound("sword sound", "/Game/Sound/melee sounds/sword sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("step_rock_l", "/Game/Sound/sfx_step_rock_l.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("step_rock_r", "/Game/Sound/sfx_step_rock_r.flac", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);
		Engine::ResourceManager::GetInst()->CreateSound("melee sound", "/Game/Sound/melee sounds/melee sound.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_3D, false);

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
		Engine::StaticCreateBindable<Engine::Mesh>("Idle", "/Game/Mesh/Idle.mesh", MESH_PATH);
		Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "/Game/Mesh/Idle2.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("Frog", "Frog.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("sword", "Sword.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("shovel", "Shovel.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("armor", "Armor_Leather.mesh", MESH_PATH);
		//Engine::StaticCreateBindable<Engine::Mesh>("pistol", "Pistol_5.mesh", MESH_PATH);
		return true;
	}

	bool GameScene::CreateTerrain()
	{// Phase E5 — Terrain is a GameObject now.
		if (auto pTerrain = CreateGameObject<Engine::Terrain>("Terrain", FindLayer(DEFAULT_LAYER)))
		{
			// Phase E7 — height map keeps Create* (different file path +
			// dynamic-access flag for in-game edits). Diffuse/Normal/Specular
			// were already registered in CreateTexture(); wire them in via
			// the new Set* path so we don't redundantly create-or-find here.
			pTerrain->CreateHeightMap("terrain", TEXT("/Game/Texture/terraintest.bmp"));
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

		std::shared_ptr<Engine::Mesh> pMesh = Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "/Game/Mesh/Idle2.mesh");
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

	void GameScene::SpawnEnemy()
	{
		if (!m_pVoxelWorld) return;

		auto pLayer = FindLayer(DEFAULT_LAYER);
		if (!pLayer) return;

		auto pEnemy = CreateGameObject<Enemy>("Enemy", pLayer);
		if (!pEnemy) return;

		pEnemy->SetVoxelWorld(m_pVoxelWorld.get());
		// Fixed spawn cell on the floor (y=1, above the y=0 stone slab).
		// Picked at the far edge of the demo wall so the chase visibly forces
		// the enemy to choose between detour and wall-break.
		pEnemy->SetSpawnCell(4, 24);

		if (auto pPlayer = pLayer->FindGameObject("player"))
		{
			pEnemy->SetTarget(pPlayer);
		}
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

		// Static weapon catalogue. The DB is a singleton so Player and
		// LevelUpChoices both find the same rows without threading a
		// pointer through. CSVLoader::Load resolves the virtual /Game/...
		// path via PathManager internally — /Game/ is mounted at
		// <exe-dir>\Resource\ by CPathManager::Init, so this works
		// regardless of the host .exe's working directory.
		WeaponDatabase::GetInst().LoadFromCSV("/Game/Data/Weapons/weapons.csv");

		Engine::ResourceManager::GetInst()->LoadSkeleton("/Game/Mesh/Idle.skel");
		//Engine::ResourceManager::GetInst()->LoadSkeleton("Frog.skel");

		LoadSequences();

		CreateSounds();

		//std::shared_ptr<Engine::Sound> pSound = Engine::ResourceManager::GetInst()->CreateSound("TownTheme", "TownTheme.mp3", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, true);

		//pSound->Play();

		CreateTexture();

		AddLayer(DEFAULT_LAYER);

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
				pLight->GetTransform()->SetRX(1.f);
			}
		}

		// Phase E5 — Player is a GameObject now.
		// Phase V6 — VoxelWorld lives on the scene. Seed it BEFORE Player
		// is created so the player can be handed a ready-to-use world.
		// Phase V7 — design pivot: voxels are 2D walls only. The world has
		// a floor at y=0 (48×48 stone slab) and walls at y=kWallY; any
		// other y is unused.
		m_pVoxelWorld = std::make_unique<Engine::VoxelWorld>(
			this, FindLayer(DEFAULT_LAYER));

		for (int x = 0; x < 48; ++x)
			for (int z = 0; z < 48; ++z)
				m_pVoxelWorld->SetBlock(x, 0, z, Engine::BlockType::Stone);

		// Demo wall: a stone strip at x=24 spanning z=14..34. Enemies that
		// spawn on one side of it and chase the player on the other side
		// will choose "break a wall block" if it beats the detour cost.
		for (int z = 14; z <= 34; ++z)
		{
			m_pVoxelWorld->SetBlock(24, kWallY, z, Engine::BlockType::Stone);
		}

		std::shared_ptr<Player> pPlayer = CreateGameObject<Player>("player", FindLayer(DEFAULT_LAYER), 100, 10, 15);
		if (pPlayer) pPlayer->SetVoxelWorld(m_pVoxelWorld.get());

		// HPBar — UIControl-derived Component on a dedicated GameObject.
		// Its child UIRenderers self-register with RenderManager's UI
		// layer (UIRenderer::PreDraw), so Scene only seeds the target
		// here and does not re-register anything per frame.
		if (auto pHPBarObj = CreateGameObject<>("HPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pHPBar = pHPBarObj->AddComponent<HPBar>("hpbar"))
			{
				pHPBar->SetTarget(pPlayer);
			}
		}

		// XP gauge — same pattern, sits just above the HP bar.
		if (auto pXPBarObj = CreateGameObject<>("XPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pXPBar = pXPBarObj->AddComponent<XPBar>("xpbar"))
			{
				pXPBar->SetTarget(pPlayer);
			}
		}

		// Level-up choice modal — hidden until Player::HasPendingLevelUp
		// flips on. Pauses the game (Engine::Window::Stop) and waits
		// for the 1/2/3 keypress to apply a boost and resume.
		if (auto pLevelUpObj = CreateGameObject<>("LevelUpChoices", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pLevelUp = pLevelUpObj->AddComponent<LevelUpChoices>("levelup"))
			{
				pLevelUp->SetTarget(pPlayer);
			}
		}

		// Debug overlay — live enemy count. Reads the active scene's
		// default layer each frame, so it works whether the Editor or
		// standalone Game launched us.
		m_pEnemyCountHUD = std::make_unique<EnemyCountHUD>();

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

		Engine::RenderManager::GetInst()->SetFogColor({ 0.f, 43.f / 255.f, 152.f / 255.f });
		Engine::RenderManager::GetInst()->SetFogHighlightColor({ 1.f, 0.f, 0.f });
		Engine::RenderManager::GetInst()->SetFogStartDepth(50.f);   // was 10 — fog 시작 거리 늘림
		Engine::RenderManager::GetInst()->SetFogDensity(0.005f);    // was 0.03 — 농도 약하게
		Engine::RenderManager::GetInst()->SetFogHeightFallOff(0.001f);

		return true;
	}

	void GameScene::Update(float dt)
	{
		__super::Update(dt);   // Scene::Update auto-advances V2 drawables.

		// HPBar's child UIRenderers self-register via their own PreDraw.
		// EnemyCountHUD stays a plain class so we still drive it here.
		if (m_pEnemyCountHUD)
		{
			EnemyCountHUD* pHud = m_pEnemyCountHUD.get();
			Engine::RenderManager::GetInst()->AddCustomRender(
				Engine::RENDER_LAYER::UI,
				[pHud]() { pHud->Render(); });
		}

		// Periodic enemy spawning — pick a random angle around the player
		// every m_fEnemySpawnInterval seconds and drop a slow-chase Enemy
		// on the voxel surface at that bearing.
		if (!m_pVoxelWorld) return;

		m_fEnemySpawnAcc += dt;
		if (m_fEnemySpawnAcc < m_fEnemySpawnInterval) return;
		m_fEnemySpawnAcc -= m_fEnemySpawnInterval;

		auto pLayer = FindLayer(DEFAULT_LAYER);
		if (!pLayer) return;

		auto pPlayer = pLayer->FindGameObject("player");
		if (!pPlayer) return;
		auto pPlayerTr = pPlayer->GetComponent<Engine::Transform>();
		if (!pPlayerTr) return;

		const float fAngle =
			(rand() / static_cast<float>(RAND_MAX)) * 2.f * PI;
		const Engine::Vector3 vPlayer = pPlayerTr->GetPosition();
		const float fX = vPlayer.x + cosf(fAngle) * m_fEnemySpawnRadius;
		const float fZ = vPlayer.z + sinf(fAngle) * m_fEnemySpawnRadius;

		// Clamp to the test scene's 48×48 base stone slab so the spawned
		// cell is always inside the navigable voxel volume.
		const int cx = std::max(0, std::min(47,
			static_cast<int>(std::floor(fX))));
		const int cz = std::max(0, std::min(47,
			static_cast<int>(std::floor(fZ))));

		auto pEnemy = CreateGameObject<Enemy>("Enemy", pLayer);
		if (!pEnemy) return;

		// Alternate between the original box mesh and the new capsule mesh
		// so both variants show up in the demo. Even spawns -> box, odd ->
		// capsule (also recoloured green inside Enemy::SetMeshKind).
		++m_iEnemySpawnIdx;
		if ((m_iEnemySpawnIdx & 1) != 0)
			pEnemy->SetMeshKind(Enemy::MESH_KIND::CAPSULE);

		pEnemy->SetVoxelWorld(m_pVoxelWorld.get());
		pEnemy->SetSpawnCell(cx, cz);
		pEnemy->SetSpeed(m_fEnemyTestSpeed);
		pEnemy->SetTarget(pPlayer);
	}

	void GameScene::Draw()
	{
		__super::Draw();
	}
}
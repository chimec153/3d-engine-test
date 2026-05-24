#include "GameScene.h"
#include "Core/ObjectFactory.h"
#include "Voxel/VoxelWorld.h"
REGISTER_SCENE(Client::GameScene, GameScene)
#include "../Object/Player.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/EnemyDatabase.h"
#include "../Object/SpawnConfig.h"
#include "EnemySpawner.h"
#include "GameWorldBuilder.h"
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
#include "UI/Gauge.h"
#include "Core/Window.h"
#include "../UI/LevelUpChoices.h"
#include "../UI/EnemyCountHUD.h"
#include "../UI/DamageText.h"
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
		return true;
	}

	bool GameScene::CreateTexture()
	{
		Engine::StaticCreateBindable<Engine::Texture>("PaperBurn", TEXT("/Game/Texture/DefaultBurn.png"), TEXTURE_PATH, 4);

		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodAlbedo", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Albedo.tga"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodNormal", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Normal.tga"), TEXTURE_PATH, 1);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodOpacity", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Opacity.tga"), TEXTURE_PATH, 2);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodRoughness", TEXT("/Game/Texture/Decal/sgfjdepc_8K_Roughness.tga"), TEXTURE_PATH, 3);
		return true;
	}

	bool GameScene::CreateSounds()
	{
		//Engine::ResourceManager::GetInst()->CreateSound("leather_inventory", "/Game/Sound/inventory_sound_effects/leather_inventory.wav", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, false);

		return true;
	}

	bool GameScene::CreateMesh()
	{
		Engine::StaticCreateBindable<Engine::Mesh>("Idle", "/Game/Mesh/Idle.mesh", MESH_PATH);
		Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "/Game/Mesh/Idle2.mesh", MESH_PATH);
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

		// Enemy catalogue + global spawn parameters. Same /Game mount as
		// weapons.csv. Both stay at compiled-in defaults if the file is
		// missing, so the game keeps running even when the CSV is wiped.
		EnemyDatabase::GetInst().LoadFromCSV("/Game/Data/Enemies/enemies.csv");
		SpawnConfig::GetInst().LoadFromCSV ("/Game/Data/Enemies/spawn.csv");

		// Floating combat text — bake the glyph atlas once.
		DamageTextManager::GetInst()->Init();

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

				// Shrink shadow ortho to match this scene's scale. Player is
				// authored at scale 0.01 (Player.cpp:442), so gameplay
				// objects sit in a ~50-unit play radius. PointLight's
				// default ortho is ±2500 — at a 2048² shadow map that's
				// ~2.4 units/texel, so a ~1-unit player casts a shadow only
				// 1-2 texels wide. Tighten to ±50 for ~0.05 units/texel.
				float fSize = 15.f;
				Engine::ORTHOINFO tOrtho = {};
				tOrtho.fLeft   = -fSize;
				tOrtho.fRight  =  fSize;
				tOrtho.fTop    =  fSize;
				tOrtho.fBottom = -fSize;
				tOrtho.fNear   =   0.1f;
				tOrtho.fFar    = 200.f;
				pLight->SetOrthoInfo(tOrtho);

				Engine::Graphics::GetInst()->SetLight(pLight);
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
		GameWorldBuilder::StampTestScene(*m_pVoxelWorld);

		// Per-frame enemy spawning lives in a dedicated helper so
		// GameScene's Update reads as a small list of dispatches.
		m_pEnemySpawner = std::make_unique<EnemySpawner>(this, m_pVoxelWorld.get());

		std::shared_ptr<Player> pPlayer = CreateGameObject<Player>("player", FindLayer(DEFAULT_LAYER), 100, 10, 15);
		if (pPlayer) pPlayer->SetVoxelWorld(m_pVoxelWorld.get());
		m_pPlayer = pPlayer;

		// HP / XP gauges. Layout is anchor-bound once (UIControl
		// re-resolves the pixel rect on Window resize); only the ratio
		// is pushed each frame from Update. Colors here match the old
		// HPBar (red on dark grey) / XPBar (yellow on darker grey)
		// constants verbatim. Anchor (0.025, 0.975) = 2.5% in from
		// bottom-left corner; pivot (0,1) makes that the bar's own
		// bottom-left, so the bar grows up-and-right from there.
		if (auto pHPObj = CreateGameObject<>("HPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pHP = pHPObj->AddComponent<Engine::Gauge>("hpbar"))
			{
				pHP->SetColors(0xFF303030, 0xFF2030E0);
				pHP->SetRectByAnchorFrac(
					Engine::Vector2{ 0.025f, 0.975f },
					Engine::Vector2{ 0.f,    1.f    },
					Engine::Vector2{ 0.2f,   0.025f });
				m_pHPGauge = pHP;
			}
		}

		if (auto pXPObj = CreateGameObject<>("XPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pXP = pXPObj->AddComponent<Engine::Gauge>("xpbar"))
			{
				pXP->SetColors(0xFF202020, 0xFF20D0E0);
				pXP->SetRectByAnchorFrac(
					Engine::Vector2{ 0.025f, 0.945f },
					Engine::Vector2{ 0.f,    1.f    },
					Engine::Vector2{ 0.2f,   0.0125f });
				m_pXPGauge = pXP;
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

		// HP / XP gauges — only the ratio needs per-frame push; the rect
		// is anchor-bound at gauge creation and re-resolved by UIControl
		// on Window resize.
		if (auto pPlayer = m_pPlayer.lock())
		{
			if (auto pHP = m_pHPGauge.lock())
			{
				const float fMax = static_cast<float>(pPlayer->GetMaxHP());
				pHP->SetRatio(fMax > 0.f ? pPlayer->GetHP() / fMax : 0.f);
			}

			if (auto pXP = m_pXPGauge.lock())
			{
				const float fNext = static_cast<float>(pPlayer->GetXpToNext());
				pXP->SetRatio(fNext > 0.f ? pPlayer->GetExp() / fNext : 0.f);
			}
		}

		// EnemyCountHUD stays a plain class so we still drive it here.
		if (m_pEnemyCountHUD)
		{
			EnemyCountHUD* pHud = m_pEnemyCountHUD.get();
			Engine::RenderManager::GetInst()->AddCustomRender(
				Engine::RENDER_LAYER::UI,
				[pHud]() { pHud->Render(); });
		}

		// Floating combat text — advance the pool every frame and queue
		// its UI draw. m_CustomRenderList is cleared each frame so the
		// AddCustomRender call needs to repeat.
		DamageTextManager::GetInst()->Update(dt);
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::UI,
			[]() { DamageTextManager::GetInst()->Render(); });

		if (m_pEnemySpawner) m_pEnemySpawner->Tick(dt);
	}

	void GameScene::Draw()
	{
		__super::Draw();
	}
}
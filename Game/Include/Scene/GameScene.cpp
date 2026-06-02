#include "GameScene.h"
#include "Core/ObjectFactory.h"
#include "Voxel/VoxelWorld.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
REGISTER_SCENE(Client::GameScene, GameScene)
#include "../Object/Player.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/TowerData.h"
#include "../Object/EnemyDatabase.h"
#include "../Object/RoundDatabase.h"
#include "../Object/LevelUpDatabase.h"
#include "../Object/Enemy.h"
#include "EnemySpawner.h"
#include "../Util/Telemetry.h"
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
#include "Resource/Font.h"
#include "Resource/FontManager.h"
#include "Resource/Text.h"
#include "Core/Window.h"
#include "../UI/GameOverUI.h"
#include "../UI/PauseMenuUI.h"
#include "../UI/EnemyCountHUD.h"
#include "../Object/GameStateManager.h"
#include "../UI/DamageText.h"
#include "../UI/WeaponHUD.h"
#include "../UI/TowerHUD.h"
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
#include "../Object/EnemyMeshRenderer.h"
#include "../Object/Vfx/VfxManager.h"
#include "../Object/Vfx/FootstepManager.h"
#include "../Object/Vfx/BeamRenderManager.h"
#include "../Object/Vfx/TrailRenderManager.h"
#include "../Object/Vfx/DeathBurstManager.h"
#include "../Object/Vfx/MuzzleFlashManager.h"
#include "../Object/Vfx/FragmentShatterManager.h"
#include "../Object/Vfx/HealAuraManager.h"
#include "../Object/Vfx/SpawnTelegraphManager.h"
#include "../Object/TowerManager.h"
#include "../Object/TowerPlacementController.h"
#include "../Object/Wallet.h"
#include "../Object/Orb.h"
#include "../UI/TowerIntermissionUI.h"
#include "../UI/StartWeaponSelectUI.h"
#include "../UI/LevelUpChoices.h"
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
	GameScene::~GameScene()
	{
		// Catch-all telemetry for mid-run exits (window close / return to menu).
		// If a run_end was already sent (death), IsRunActive() is false -> no-op.
		SendRunEndTelemetry("quit");
	}

	// Telemetry: assemble run_end (round / level / weapons) and send with the
	// given reason. No-op if no run is active, which also prevents duplicate
	// sends since RunEnd clears the run_id.
	void GameScene::SendRunEndTelemetry(const char* szReason)
	{
		auto& tm = Telemetry::GetInst();
		if (!tm.IsRunActive()) return;

		std::vector<std::string> vecWeapons;
		int iLevel = 1;
		if (auto pPlayer = m_pPlayer.lock())
		{
			iLevel = pPlayer->GetLevel();
			for (int id : pPlayer->GetOwnedWeaponIds())
				vecWeapons.push_back(std::to_string(id));
		}
		const int iRound = m_pEnemySpawner ? m_pEnemySpawner->GetRound() : m_iRound;
		tm.RunEnd(iRound, iLevel, m_fActivePlayTime, vecWeapons, szReason);
	}

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
		//Engine::StaticCreateBindable<Engine::Mesh>("Idle2", "/Game/Mesh/Idle2.mesh", MESH_PATH);
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
		// A previous run may have left a modal active (e.g. died → game-over →
		// menu). Force back to Playing so this new game starts unpaused — the
		// global timer would otherwise still be stopped and the game frozen.
		GameStateManager::GetInst().ExitModal();

		// Phase E7 — preload Walking.fbx mesh data into BindableManager via
		// the MeshLoader facade. Drawable's loader bridge has been retired;
		// MeshLoader::Load drives the same parser pipeline without a temp
		// Drawable, and the parsed Bindables register in BindableManager.
		//Engine::MeshLoader::Load(TEXT("Idle.fbx"));
		//Engine::MeshLoader::Load(TEXT("Standing Melee Attack Downward.fbx"));
		//Engine::MeshLoader::Load(TEXT("Slow Run.fbx"));

		CreateMesh();

		// Register the game-side enemy shaders + 256-byte instance input
		// layout before any enemy spawns. Enemies render through these
		// (EnemyMeshRenderer) so same-kind enemies batch into one
		// DrawInstanced call while keeping per-instance hit flash + dissolve.
		EnemyMeshRenderer::RegisterShaders();

		// Static weapon catalogue. The DB is a singleton so Player and
		// LevelUpChoices both find the same rows without threading a
		// pointer through. CSVLoader::Load resolves the virtual /Game/...
		// path via PathManager internally — /Game/ is mounted at
		// <exe-dir>\Resource\ by CPathManager::Init, so this works
		// regardless of the host .exe's working directory.
		WeaponDatabase::GetInst().LoadFromCSV("/Game/Data/Weapons/weapons_v2.csv");

		// Cross-run weapon unlocks: ids the player has acquired in past runs,
		// offered in the start-of-game picker. Guarded to load once per process;
		// missing file => nothing unlocked yet (first run).
		WeaponDatabase::GetInst().LoadUnlocked("/Game/Data/Weapons/unlocked.csv");

		// Tower stat catalogue (base HP / attack / defense / fire-rate / crit /
		// range / price, plus heal-tower params). Same /Game mount; Tower,
		// HealTower and the shop read it, falling back to the GameDefs
		// constants when a row/column is missing.
		TowerDatabase::GetInst().LoadFromCSV("/Game/Data/towers.csv");

		// Enemy catalogue + round schedule, both JSON. Same /Game mount as
		// weapons.csv. enemies.json defines the archetype catalogue (HP,
		// speed, contact damage, gold/xp reward, hitbox); rounds.json
		// drives the per-round spawn windows + hp/damage multipliers.
		EnemyDatabase::GetInst().LoadFromJSON("/Game/Data/Enemies/enemies.json");
		RoundDatabase::GetInst().LoadFromJSON("/Game/Data/Enemies/rounds.json");

		// Level-up card catalogue (data-driven stat-upgrade menu).
		LevelUpDatabase::GetInst().LoadFromCSV("/Game/Data/levelups.csv");

		// Floating combat text — bake the glyph atlas once.
		DamageTextManager::GetInst()->Init();

		// Player footstep marks — build the footprint texture + reset the pool.
		FootstepManager::GetInst()->Init();

		// Heal-tower ground-circle VFX — build the circle texture + resources.
		HealAuraManager::GetInst()->Init();

		// Pre-spawn ground-circle warnings — build the ring + disc textures.
		SpawnTelegraphManager::GetInst()->Init();

		// Laser beam billboards — resolve the BeamVS/BeamPS render resources.
		BeamRenderManager::GetInst()->Init();

		// Bullet tracer trails — reuse the beam pipeline (ribbon + head glow).
		TrailRenderManager::GetInst()->Init();

		// Enemy-death bursts — puff cloud / sparkles / smoke ring (ramp LUT).
		DeathBurstManager::GetInst()->Init();

		// Muzzle flashes — procedural 8-spoke starburst billboards (reuse beam pipeline).
		MuzzleFlashManager::GetInst()->Init();

		Engine::ResourceManager::GetInst()->LoadSkeleton("/Game/Mesh/Idle.skel");
		//Engine::ResourceManager::GetInst()->LoadSkeleton("Frog.skel");

		LoadSequences();

		CreateSounds();

		//std::shared_ptr<Engine::Sound> pSound = Engine::ResourceManager::GetInst()->CreateSound("TownTheme", "TownTheme.mp3", SOUND_PATH, 0.5f, 5000.f, FMOD_2D, true);

		//pSound->Play();

		CreateTexture();

		AddLayer(DEFAULT_LAYER);

		// Shared hit / death particle-burst pool. One host GameObject carries a
		// small emitter pool (VfxManager) so per-enemy particle cost is avoided.
		if (auto pVfx = CreateGameObject("Vfx", FindLayer(DEFAULT_LAYER)))
			VfxManager::GetInst()->Setup(pVfx.get());

		// GPU mesh-particle death-shatter pool. Owns its own D3D resources
		// (no host GameObject); Setup is idempotent across scene reloads.
		FragmentShatterManager::GetInst()->Setup();

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

		// Fresh stage starts with no money — orbs (enemy drops) earn it and
		// the between-round shop spends it. Round 1 is NOT started here: the
		// player first picks a starting weapon in the StartSelect panel (wired
		// below), and that pick kicks off round 1.
		Wallet::GetInst().Reset();
		// Tower inventory must reset per run too (the TowerManager singleton
		// outlives the scene); otherwise bought towers + their reserve weapon
		// queue carry over after a game over.
		TowerManager::GetInst().Reset();
		// One free starting tower; more are bought in the shop. The placement
		// controller lets you place up to this many.
		//TowerManager::GetInst().SetTowersOwned(10);
		//TowerManager::GetInst().SetHealTowersOwned(10);   // buy heal towers in the shop

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

		// XP bar — just below the HP bar (yellow on darker grey). Same
		// left anchor + width; placed a hair under the HP gauge's bottom
		// edge (HP spans y 0.95..0.975, so XP sits at 0.978..0.992).
		if (auto pXPObj = CreateGameObject<>("XPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pXP = pXPObj->AddComponent<Engine::Gauge>("xpbar"))
			{
				pXP->SetColors(0xFF202020, 0xFF00D0F0);
				pXP->SetRectByAnchorFrac(
					Engine::Vector2{ 0.025f, 0.992f },
					Engine::Vector2{ 0.f,    1.f    },
					Engine::Vector2{ 0.2f,   0.014f });
				m_pXPGauge = pXP;
			}
		}

		// Gold readout — replaces the XP bar. A left-aligned text label just
		// above the HP gauge; updated each frame from Wallet::Money().
		if (auto pMoneyObj = CreateGameObject<>("MoneyHUD", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pMoney = pMoneyObj->AddComponent<Engine::Text>("money"))
			{
				auto pFont = Engine::FontManager::GetInst()->CreateFont(
					"hud_money", L"Arial", 22.f, DWRITE_FONT_WEIGHT_BOLD);
				pMoney->SetFont(pFont);
				pMoney->SetColor(0xFFD000FFu);   // gold (RRGGBBAA)
				pMoney->SetHAlign(Engine::Text::HAlign::Left);
				pMoney->SetVAlign(Engine::Text::VAlign::Center);
				pMoney->SetRectByAnchorFrac(
					Engine::Vector2{ 0.025f, 0.945f },
					Engine::Vector2{ 0.f,    1.f    },
					Engine::Vector2{ 0.45f,  0.035f });
				pMoney->SetString(L"Gold: 0");
				m_pMoneyText = pMoney;
			}
		}

		// Round survival countdown — top-centre. Updated each frame from the
		// spawner's remaining time; survive it to clear the round.
		if (auto pTimerObj = CreateGameObject<>("RoundTimerHUD", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pTimer = pTimerObj->AddComponent<Engine::Text>("roundtimer"))
			{
				auto pFont = Engine::FontManager::GetInst()->CreateFont(
					"hud_round", L"Arial", 26.f, DWRITE_FONT_WEIGHT_BOLD);
				pTimer->SetFont(pFont);
				pTimer->SetColor(0xFFFFFFFFu);
				pTimer->SetHAlign(Engine::Text::HAlign::Center);
				pTimer->SetVAlign(Engine::Text::VAlign::Center);
				pTimer->SetRectByAnchorFrac(
					Engine::Vector2{ 0.5f,  0.03f },
					Engine::Vector2{ 0.5f,  0.f   },
					Engine::Vector2{ 0.4f,  0.045f });
				pTimer->SetString(L"");
				m_pRoundTimerText = pTimer;
			}
		}

		// Boss HP bar — wide bar just under the round timer, with a name caption
		// above it. Created hidden; Update enables both while a boss is alive and
		// pushes the ratio (boss HP / max HP). Crimson fill on dark grey.
		if (auto pBossObj = CreateGameObject<>("BossHPBar", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pBoss = pBossObj->AddComponent<Engine::Gauge>("bosshp"))
			{
				pBoss->SetColors(0xFF202020u, 0xFF2828D8u);   // 0xAABBGGRR: crimson fill
				pBoss->SetRectByAnchorFrac(
					Engine::Vector2{ 0.5f,  0.12f },
					Engine::Vector2{ 0.5f,  0.f   },
					Engine::Vector2{ 0.5f,  0.022f });
				pBoss->Disable();
				m_pBossHPGauge = pBoss;
			}
		}
		if (auto pBossNameObj = CreateGameObject<>("BossNameHUD", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pName = pBossNameObj->AddComponent<Engine::Text>("bossname"))
			{
				auto pFont = Engine::FontManager::GetInst()->CreateFont(
					"hud_boss", L"Arial", 22.f, DWRITE_FONT_WEIGHT_BOLD);
				pName->SetFont(pFont);
				pName->SetColor(0xFFC050FFu);
				pName->SetHAlign(Engine::Text::HAlign::Center);
				pName->SetVAlign(Engine::Text::VAlign::Center);
				pName->SetRectByAnchorFrac(
					Engine::Vector2{ 0.5f,  0.085f },
					Engine::Vector2{ 0.5f,  0.f    },
					Engine::Vector2{ 0.5f,  0.035f });
				pName->SetString(L"");
				pName->Disable();
				m_pBossNameText = pName;
			}
		}

		// Installable-tower hotbar (bottom-right). One solid colour icon per
		// tower type (a full Gauge = a flat colour quad), the remaining count
		// centred on it, and a hotkey label below. Counts are pushed each frame
		// in Update; colours match the placement ghost (blue=attack, green=heal).
		// Gauge::SetColors is 0xAABBGGRR (R = lowest byte).
		struct TowerSlotDef { float fX; uint32_t uColor; const wchar_t* szLabel; };
		const TowerSlotDef kSlots[kTowerSlots] = {
			{ 0.900f, 0xFFF06020u, L"1 Atk"  },   // attack — blue
			{ 0.955f, 0xFF40C020u, L"2 Heal" },   // heal   — green
		};
		for (int i = 0; i < kTowerSlots; ++i)
		{
			if (auto pIconObj = CreateGameObject<>("TowerIcon" + std::to_string(i), FindLayer(DEFAULT_LAYER)))
			{
				if (auto pIcon = pIconObj->AddComponent<Engine::Gauge>("towericon"))
				{
					pIcon->SetColors(0xFF202020u, kSlots[i].uColor);
					pIcon->SetRatio(1.f);   // full fill = solid colour square
					pIcon->SetRectByAnchorFrac(
						Engine::Vector2{ kSlots[i].fX, 0.880f },
						Engine::Vector2{ 0.5f,         1.f    },
						Engine::Vector2{ 0.04f,        0.06f  });
					m_pTowerIcon[i]      = pIcon;
					m_uTowerIconColor[i] = kSlots[i].uColor;
				}
			}
			if (auto pCntObj = CreateGameObject<>("TowerCount" + std::to_string(i), FindLayer(DEFAULT_LAYER)))
			{
				if (auto pCnt = pCntObj->AddComponent<Engine::Text>("towercount"))
				{
					auto pFont = Engine::FontManager::GetInst()->CreateFont(
						"hud_towercount", L"Arial", 24.f, DWRITE_FONT_WEIGHT_BOLD);
					pCnt->SetFont(pFont);
					pCnt->SetColor(0xFFFFFFFFu);
					pCnt->SetHAlign(Engine::Text::HAlign::Center);
					pCnt->SetVAlign(Engine::Text::VAlign::Center);
					pCnt->SetRectByAnchorFrac(
						Engine::Vector2{ kSlots[i].fX, 0.850f },
						Engine::Vector2{ 0.5f,         0.5f   },
						Engine::Vector2{ 0.05f,        0.05f  });
					pCnt->SetString(L"x0");
					m_pTowerCount[i] = pCnt;
				}
			}
			if (auto pLblObj = CreateGameObject<>("TowerLabel" + std::to_string(i), FindLayer(DEFAULT_LAYER)))
			{
				if (auto pLbl = pLblObj->AddComponent<Engine::Text>("towerlabel"))
				{
					auto pFont = Engine::FontManager::GetInst()->CreateFont(
						"hud_towerlabel", L"Arial", 14.f, DWRITE_FONT_WEIGHT_BOLD);
					pLbl->SetFont(pFont);
					pLbl->SetColor(0xFFFFFFFFu);
					pLbl->SetHAlign(Engine::Text::HAlign::Center);
					pLbl->SetVAlign(Engine::Text::VAlign::Center);
					pLbl->SetRectByAnchorFrac(
						Engine::Vector2{ kSlots[i].fX, 0.885f },
						Engine::Vector2{ 0.5f,         0.f    },
						Engine::Vector2{ 0.06f,        0.025f });
					pLbl->SetString(kSlots[i].szLabel);
				}
			}
		}

		// Level-up modal — orbs grant XP; on a level-up it shows 3 random
		// stat upgrades (move speed / max HP / attack / crit / defense) and
		// pauses until the player picks one.
		if (auto pLevelUpObj = CreateGameObject<>("LevelUpChoices", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pLevelUp = pLevelUpObj->AddComponent<LevelUpChoices>("levelup"))
			{
				pLevelUp->SetTarget(pPlayer);
			}
		}

		// Tower placement controller — press 1 to toggle the ghost-preview
		// placement mode. Borrows the voxel world (wall rejection + handed to
		// each spawned tower). The scene keeps a weak ref so Update can
		// register the ghost's ALPHA render callback every frame.
		if (auto pPlaceObj = CreateGameObject<>("TowerPlacement", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pPlace = pPlaceObj->AddComponent<TowerPlacementController>("placement"))
			{
				pPlace->SetVoxelWorld(m_pVoxelWorld.get());
				m_pPlacement = pPlace;
			}
		}

		// Between-round intermission panel — weapon select + start button.
		// Round-number provider returns the NEXT round; start advances the
		// round and resumes play.
		if (auto pInterObj = CreateGameObject<>("TowerIntermission", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pInter = pInterObj->AddComponent<TowerIntermissionUI>("intermission"))
			{
				pInter->SetTarget(pPlayer);
				pInter->SetRoundNumberProvider([this]() { return m_iRound + 1; });
				pInter->SetOnStartNextRound([this]()
				{
					++m_iRound;
					// Towers destroyed last round come back online this round
					// (the destroy-cooldown is "until the next round").
					TowerManager::GetInst().OnNewRound();
					if (m_pEnemySpawner) m_pEnemySpawner->StartRound(m_iRound);
					GameStateManager::GetInst().ExitModal();
				});
				m_pIntermission = pInter;
			}
		}

		// Start-of-game weapon picker. The pick arms the player + towers and
		// starts round 1 (via OnChosen). Hidden unless the StartSelect modal
		// is entered just below.
		if (auto pStartObj = CreateGameObject<>("StartWeaponSelect", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pStart = pStartObj->AddComponent<StartWeaponSelectUI>("startselect"))
			{
				pStart->SetTarget(pPlayer);
				pStart->SetOnChosen([this]()
				{
					m_iRound = 1;
					m_fActivePlayTime = 0.f;           // reset active-time accumulator for the new run
					Telemetry::GetInst().RunStart();   // new run: fresh run_id + run_start event
					if (m_pEnemySpawner) m_pEnemySpawner->StartRound(m_iRound);
					GameStateManager::GetInst().ExitModal();
				});
			}
		}

		// Decide the opening flow. With crafted weapons available, freeze on
		// the picker (round 1 starts when the player chooses). Otherwise seed
		// Arrow (id 1) for both player and towers and start round 1 straight
		// away — nothing to pick.
		/*if (WeaponDatabase::GetInst().AllCraftedLiveIds().empty())
		{
			TowerManager::GetInst().SetCurrentWeaponId(1);
			if (pPlayer) pPlayer->AddOrLevelUpWeapon(1);
			m_iRound = 1;
			m_pEnemySpawner->StartRound(m_iRound);
		}
		else*/
		{
			// Default the tower weapon (overridden by the pick); freeze the
			// game on the picker until a weapon is chosen.
			/*TowerManager::GetInst().SetCurrentWeaponId(
				WeaponDatabase::GetInst().AllCraftedLiveIds().front());*/
			GameStateManager::GetInst().EnterModal(GameState::StartSelect);
		}

		// Game-over overlay — hidden until the player's HP hits 0, then it
		// pauses the game and offers a button back to the start screen.
		if (auto pGameOverObj = CreateGameObject<>("GameOverUI", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pGameOver = pGameOverObj->AddComponent<GameOverUI>("gameover"))
			{
				pGameOver->SetTarget(pPlayer);
			}
		}

		// ESC pause menu — hidden until ESC, then dims the game and offers
		// 이어하기 (resume) / 종료하기 (back to the title screen).
		if (auto pPauseObj = CreateGameObject<>("PauseMenuUI", FindLayer(DEFAULT_LAYER)))
		{
			pPauseObj->AddComponent<PauseMenuUI>("pausemenu");
		}

		// Top-left HUD listing owned weapons + current level. Polls
		// Player::GetOwnedWeaponIds / GetOwnedWeaponLevel each frame.
		if (auto pWeaponHUDObj = CreateGameObject<>("WeaponHUD", FindLayer(DEFAULT_LAYER)))
		{
			if (auto pWeaponHUD = pWeaponHUDObj->AddComponent<WeaponHUD>("weaponhud"))
			{
				pWeaponHUD->SetTarget(pPlayer);
			}
		}

		// Tower slots HUD — top-right corner, one box per owned tower (placed /
		// ready / on destroy-cooldown). Reads TowerManager + the live "Tower"
		// objects each frame; no target needed.
		if (auto pTowerHUDObj = CreateGameObject<>("TowerHUD", FindLayer(DEFAULT_LAYER)))
		{
			pTowerHUDObj->AddComponent<TowerHUD>("towerhud");
		}

		// Debug overlay — live enemy count. Reads the active scene's
		// default layer each frame, so it works whether the Editor or
		// standalone Game launched us.
#ifdef _DEBUG
		m_pEnemyCountHUD = std::make_shared<EnemyCountHUD>();
#endif

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
		// Drop last frame's heal-aura submissions BEFORE components update, so
		// the heal towers' Submit calls (during __super::Update) populate this
		// frame's batch for the ALPHA render below.
		HealAuraManager::GetInst()->BeginFrame();
		// Drop last frame's spawn telegraphs — EnemySpawner::Tick will re-submit
		// for every pending spawn this frame.
		SpawnTelegraphManager::GetInst()->BeginFrame();

		__super::Update(dt);   // Scene::Update auto-advances V2 drawables.

		// Telemetry: accumulate active play time only while actually playing, so
		// choice/pause screens (StartSelect, Intermission shop, LevelUp, Paused)
		// are excluded. dt is already ~0 during modals, but gate explicitly too.
		if (GameStateManager::GetInst().IsPlaying())
			m_fActivePlayTime += dt;

		// Telemetry: player death -> run_end("death"), exactly once (helper guards).
		if (auto pPlayer = m_pPlayer.lock(); pPlayer && pPlayer->IsDead())
			SendRunEndTelemetry("death");

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
				const float fNeed = static_cast<float>(pPlayer->GetXpToNext());
				pXP->SetRatio(fNeed > 0.f ? pPlayer->GetExp() / fNeed : 0.f);
			}
		}

		// Gold readout + installable-tower icon counts (independent of the
		// player lock — both are global state).
		// Gold is just the number now — the per-type tower budget moved to the
		// bottom-right icon hotbar. Count placed towers of each type once, then
		// push remaining = owned - placed into each icon's count + colour.
		int iPlacedAtk = 0, iPlacedHeal = 0;
		if (auto pLayer = FindLayer(DEFAULT_LAYER))
			for (const auto& p : pLayer->GetGameObjectList())
			{
				if (!p || !p->IsActive()) continue;
				if (p->GetTag() == "Tower")          ++iPlacedAtk;
				else if (p->GetTag() == "HealTower") ++iPlacedHeal;
			}

		if (auto pMoney = m_pMoneyText.lock())
			pMoney->SetString(L"Gold: " + std::to_wstring(Wallet::GetInst().Money()));

		const int iRemain[kTowerSlots] = {
			TowerManager::GetInst().TowersOwned()     - iPlacedAtk,
			TowerManager::GetInst().HealTowersOwned() - iPlacedHeal,
		};
		const uint32_t uActive[kTowerSlots] = { 0xFFF06020u, 0xFF40C020u };
		for (int i = 0; i < kTowerSlots; ++i)
		{
			const int iLeft = iRemain[i] > 0 ? iRemain[i] : 0;
			if (auto pCnt = m_pTowerCount[i].lock())
			{
				pCnt->SetString(L"x" + std::to_wstring(iLeft));
				pCnt->SetColor(iLeft > 0 ? 0xFFFFFFFFu : 0xFF6060FFu);   // white / muted red
			}
			// Dim the icon to grey when none are installable. SetColors swaps the
			// texture SRV, so only re-bind when the colour actually changes.
			const uint32_t uWant = iLeft > 0 ? uActive[i] : 0xFF303030u;
			if (uWant != m_uTowerIconColor[i])
			{
				if (auto pIcon = m_pTowerIcon[i].lock())
					pIcon->SetColors(0xFF202020u, uWant);
				m_uTowerIconColor[i] = uWant;
			}
		}

		// Round survival countdown — seconds left to survive (blank when not
		// in an active round, e.g. during the shop / start pick).
		if (auto pTimer = m_pRoundTimerText.lock())
		{
			if (m_pEnemySpawner &&
				GameStateManager::GetInst().IsPlaying() &&
				m_pEnemySpawner->IsRoundActive())
			{
				const int iSec = static_cast<int>(std::ceil(m_pEnemySpawner->GetRoundTimeRemaining()));
				pTimer->SetString(
					L"Round " + std::to_wstring(m_pEnemySpawner->GetRound()) +
					L"   -   Survive " + std::to_wstring(iSec) + L"s");
			}
			else
			{
				pTimer->SetString(L"");
			}
		}

		// Boss HP bar — show while the spawner's tracked boss is alive (active
		// and HP > 0); otherwise hide both the bar and the name caption.
		{
			std::shared_ptr<Enemy> pBoss =
				m_pEnemySpawner ? m_pEnemySpawner->GetBoss() : nullptr;
			const bool bShow = pBoss && pBoss->IsActive() && pBoss->GetHP() > 0;
			if (auto pGauge = m_pBossHPGauge.lock())
			{
				if (bShow)
				{
					const float fMax = static_cast<float>(pBoss->GetMaxHP());
					pGauge->SetRatio(fMax > 0.f ? pBoss->GetHP() / fMax : 0.f);
					pGauge->Enable();
				}
				else pGauge->Disable();
			}
			if (auto pName = m_pBossNameText.lock())
			{
				if (bShow)
				{
					const std::string& s = pBoss->GetName();
					pName->SetString(std::wstring(s.begin(), s.end()));
					pName->Enable();
				}
				else pName->Disable();
			}
		}

		// EnemyCountHUD + floating damage text draw through the UI
		// custom-render queue, which RenderUI runs AFTER the UIRenderer
		// components — so they'd otherwise sit on TOP of the level-up /
		// game-over modals. Skip them while a modal is up so those screens
		// read cleanly and are effectively the topmost layer (the game is
		// paused then, so hiding these transient overlays loses nothing).
		const bool bPlaying = GameStateManager::GetInst().IsPlaying();
		if (bPlaying)
		{
			// Capture a weak_ptr, not the raw pointer: a scene change (e.g.
			// the game-over button) can free this HUD between this
			// registration and when RenderUI runs the callback. lock() then
			// fails and we skip, instead of dereferencing a dangling HUD.
#ifdef _DEBUG
			// Instancing-count debug HUD: Debug builds only. Excluded from
			// Release (itch) so players never see the counters.
			if (m_pEnemyCountHUD)
			{
				std::weak_ptr<EnemyCountHUD> wpHud = m_pEnemyCountHUD;
				Engine::RenderManager::GetInst()->AddCustomRender(
					Engine::RENDER_LAYER::UI,
					[wpHud]() { if (auto p = wpHud.lock()) p->Render(); });
			}
#endif

			Engine::RenderManager::GetInst()->AddCustomRender(
				Engine::RENDER_LAYER::UI,
				[]() { DamageTextManager::GetInst()->Render(); });
		}

		// Footstep marks — world-space ground quads in the ALPHA pass (drawn
		// regardless of play state so existing marks keep fading on pause).
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { FootstepManager::GetInst()->Render(); });

		// Heal-tower aura circles — ground quads in the ALPHA pass.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { HealAuraManager::GetInst()->Render(); });

		// Laser beams — camera-facing additive billboards in the ALPHA pass.
		// Each Beam Submit()s its segment during update; this draws + drains.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { BeamRenderManager::GetInst()->Render(); });

		// Bullet tracer trails — additive ribbons + head glows in the ALPHA
		// pass. Each Bullet Submit()s its history during update; this draws.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { TrailRenderManager::GetInst()->Render(); });

		// Enemy-death bursts — puff/sparkle/ring billboards in the ALPHA pass.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { DeathBurstManager::GetInst()->Render(); });

		// Muzzle flashes — additive starburst billboards in the ALPHA pass.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { MuzzleFlashManager::GetInst()->Render(); });

		// Pre-spawn warning circles — ground quads in the ALPHA pass.
		Engine::RenderManager::GetInst()->AddCustomRender(
			Engine::RENDER_LAYER::ALPHA,
			[]() { SpawnTelegraphManager::GetInst()->Render(); });

		// Tower placement ghost — a translucent cube in the ALPHA pass while
		// placement mode is active. Weak-captured so a scene change between
		// this registration and the draw can't dangle.
		if (!m_pPlacement.expired())
		{
			std::weak_ptr<TowerPlacementController> wpPlace = m_pPlacement;
			Engine::RenderManager::GetInst()->AddCustomRender(
				Engine::RENDER_LAYER::ALPHA,
				[wpPlace]() { if (auto p = wpPlace.lock()) p->RenderGhost(); });
		}

		// Advance the damage-text pool every frame regardless (dt is ~0 while
		// paused, so nothing actually moves; this just keeps it in sync).
		DamageTextManager::GetInst()->Update(dt);
		FootstepManager::GetInst()->Update(dt);
		DeathBurstManager::GetInst()->Update(dt);
		MuzzleFlashManager::GetInst()->Update(dt);

		if (m_pEnemySpawner) m_pEnemySpawner->Tick(dt);

		// Round complete (quota spawned + every enemy dead) → freeze the game
		// and open the intermission panel. Gated on IsPlaying so it doesn't
		// re-fire while a level-up modal is already up; it opens next frame
		// once that resolves.
		if (m_pEnemySpawner &&
			GameStateManager::GetInst().IsPlaying() &&
			m_pEnemySpawner->IsRoundComplete())
		{
			// Survived the time limit — clear enemies, then start a short
			// "collecting" phase: leftover orbs magnet into the player before
			// the shop opens (the game stays unpaused so they can fly in).
			m_pEnemySpawner->EndRound();
			m_pEnemySpawner->ClearEnemies();
			// Surviving a round fully restores the player's health.
			if (auto pPlayer = m_pPlayer.lock()) pPlayer->FullHeal();
			if (auto pLayer = FindLayer(DEFAULT_LAYER))
				for (const auto& p : pLayer->GetGameObjectList())
					if (p && p->IsActive() && p->GetTag() == "Orb")
						if (auto pOrb = std::dynamic_pointer_cast<Orb>(p))
							pOrb->PullToPlayer();
			m_bCollectingOrbs = true;
			m_fCollectTimer   = 0.f;
		}

		// Collecting phase — wait for the magnet orbs to be picked up (or a
		// timeout), then freeze the game and open the intermission shop.
		if (m_bCollectingOrbs && GameStateManager::GetInst().IsPlaying())
		{
			m_fCollectTimer += dt;
			int iOrbs = 0;
			if (auto pLayer = FindLayer(DEFAULT_LAYER))
				for (const auto& p : pLayer->GetGameObjectList())
					if (p && p->IsActive() && p->GetTag() == "Orb") ++iOrbs;

			constexpr float kMaxCollectTime = 2.5f;
			if (iOrbs == 0 || m_fCollectTimer >= kMaxCollectTime)
			{
				m_bCollectingOrbs = false;
				GameStateManager::GetInst().EnterModal(GameState::Intermission);
			}
		}
	}

	void GameScene::Draw()
	{
		__super::Draw();
	}
}
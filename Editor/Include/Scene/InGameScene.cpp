#include "InGameScene.h"
#include "../Imgui/ImguiManager.h"
#include "Bindable/Camera.h"
#include "Bindable/PointLight.h"
#include "Bindable/Transform.h"
#include "Scene/Scene.h"
#include "../Object/Player.h"
#include "Bindable/Terrain.h"
#include "Bindable/ColliderMesh.h"
#include "Bindable/Particle.h"
#include "Bindable/Decal.h"
#include "Bindable/Box.h"
#include "Bindable/Topology.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Fluid.h"
#include "Bindable/SkyBox.h"
#include "Render/RenderManager.h"
#include "Bindable/Cloth.h"
#include "Bindable/Sphere.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/NavMesh.h"
#include "Input/Input.h"
#include "Bindable/ColliderOBB.h"
#include "Bindable/Animation.h"
#include "Bindable/Material.h"
#include "Bindable/BindableManager.h"
#include "Resource/ResourceManager.h"

namespace Editor
{
	InGameScene::InGameScene()
	{

	}

	bool InGameScene::Init()
	{
		Engine::StaticCreateBindable<Engine::Mesh>("Medieval", "Medieval.mesh", MESH_PATH);
		Engine::StaticCreateBindable<Engine::Mesh>("Frog", "Frog.mesh", MESH_PATH);
		Engine::StaticCreateBindable<Engine::Mesh>("armor", "Armor_Leather.mesh", MESH_PATH);

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
		Engine::StaticCreateBindable<Engine::Texture>("QuickSlot", TEXT("item.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("frame.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("frame", TEXT("frame.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("shovel_icon", TEXT("shovel_icon.png"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("sword_icon", TEXT("sword_icon.png"), TEXTURE_PATH, 0);

		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodAlbedo", TEXT("Decal\\sgfjdepc_8K_Albedo.tga"), TEXTURE_PATH, 0);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodNormal", TEXT("Decal\\sgfjdepc_8K_Normal.tga"), TEXTURE_PATH, 1);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodOpacity", TEXT("Decal\\sgfjdepc_8K_Opacity.tga"), TEXTURE_PATH, 2);
		Engine::StaticCreateBindable<Engine::Texture>("DecalBloodRoughness", TEXT("Decal\\sgfjdepc_8K_Roughness.tga"), TEXTURE_PATH, 3);

		Load("Resource\\Scene\\test.scn");

		std::shared_ptr<Engine::Drawable> pPlayer = CreateDrawable<Engine::Drawable>("player", FindLayer(DEFAULT_LAYER));

		pPlayer->Load(TEXT("UltimateModularWomenPack\\Medieval\\Medieval.fbx"));

		std::shared_ptr<Engine::Drawable> pItemDrawable = CreateDrawable<Engine::Drawable>("testDrawable", FindLayer(DEFAULT_LAYER));

		pItemDrawable->FindAndAddBind<Engine::VertexShader>("anisotropic_microfacet VSNoSkin");
		pItemDrawable->FindAndAddBind<Engine::PixelShader>("anisotropic_microfacet PS_NoDiffuseNoSpecNoNormal");
		pItemDrawable->FindAndAddBind<Engine::Topology>("TriangleList");
		pItemDrawable->FindAndAddBind<Engine::InputLayout>("Standard");
		pItemDrawable->FindAndAddBind<Engine::Mesh>("armor");

		std::shared_ptr<Engine::Material> pSrcMaterial = Engine::StaticFindBindable<Engine::Material>("Material");

		pItemDrawable->AddChild(pSrcMaterial->Clone());

		pPlayer->GetAnimation()->AddSocket(10, pItemDrawable);

		std::shared_ptr<Engine::Camera> pCamera = Engine::Graphics::GetInst()->GetCamera();

		if (!Engine::CInput::GetInst()->CreateAction("_W", DIK_W))
		{
			return false;
		}

		Engine::CInput::GetInst()->AddAction("_W", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveFront);
		if (!Engine::CInput::GetInst()->CreateAction("_S", DIK_S))
		{
			return false;
		}
		Engine::CInput::GetInst()->AddAction("_S", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveBack);
		if (!Engine::CInput::GetInst()->CreateAction("_A", DIK_A))
		{
			return false;
		}
		Engine::CInput::GetInst()->AddAction("_A", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveLeft);
		if (!Engine::CInput::GetInst()->CreateAction("_D", DIK_D))
		{
			return false;
		}
		Engine::CInput::GetInst()->AddAction("_D", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveRight);
		if (!Engine::CInput::GetInst()->CreateAction("_Q", DIK_Q))
		{
			return false;
		}
		Engine::CInput::GetInst()->AddAction("_Q", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveUp);
		if (!Engine::CInput::GetInst()->CreateAction("_E", DIK_E))
		{
			return false;
		}
		Engine::CInput::GetInst()->AddAction("_E", Engine::CInput::KEY_STATE::PRESS, pCamera.get(), &Engine::Camera::CameraMoveDown);

		return true;
	}
	void InGameScene::Update(float fDeltaTime)
	{
		__super::Update(fDeltaTime);

		int iX = Engine::CInput::GetInst()->GetMouseDeltaX();
		int iY = Engine::CInput::GetInst()->GetMouseDeltaY();

		std::shared_ptr<Engine::Camera> pCamera = Engine::Graphics::GetInst()->GetCamera();

		if (pCamera && !Engine::Window::GetInst()->IsLockRotation())
		{
			std::shared_ptr<Engine::Transform> pCamTranform = pCamera->GetTransform();

			if (pCamTranform)
			{
				pCamTranform->AddRX(iY * fDeltaTime);
				pCamTranform->AddRY(iX * fDeltaTime);
			}
		}
	}
}
#include "InGameScene.h"
#include "../Imgui/ImguiManager.h"
#include "Bindable/Camera.h"
#include "Bindable/PointLight.h"
#include "Bindable/TransformBuffer.h"
#include "Scene/Scene.h"
#include "../Object/Player.h"
#include "Bindable/Terrain.h"
#include "Bindable/ColliderMesh.h"
#include "Bindable/Particle.h"
#include "Bindable/Decal.h"
#include "Bindable/Box.h"
#include "Bindable/Topology.h"
#include "Bindable/InputLayout.h"

InGameScene::InGameScene()
{
	
}

bool InGameScene::Init()
{
	const std::shared_ptr<Engine::Camera>& pCamera = CreateDrawable<Engine::Camera>("camera", FindLayer(DEFAULT_LAYER));

	Engine::Graphics::GetInst()->SetCamera(pCamera);

	const std::shared_ptr<Engine::PointLight>& pDirLight = CreateDrawable<Engine::PointLight>("light2", FindLayer(DEFAULT_LAYER));

	std::shared_ptr<Engine::Transform> pDirLightTransform = pDirLight->GetTransform();

	pDirLightTransform->SetRX(0.962f);
	pDirLightTransform->SetX(15.f);
	pDirLightTransform->SetY(50.f);
	pDirLightTransform->SetZ(-20.f);

	Engine::ORTHOINFO tLightOrthoInfo = pDirLight->GetOrthoInfo();

	tLightOrthoInfo.fLeft = -0.f;
	tLightOrthoInfo.fRight = 0.f;
	tLightOrthoInfo.fTop = 0.f;
	tLightOrthoInfo.fBottom = 0.f;

	pDirLight->SetOrthoInfo(tLightOrthoInfo);

	pDirLight->SetIntensity(3.f);

	pDirLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);

	Engine::Graphics::GetInst()->SetLight(pDirLight);

	const std::shared_ptr<Engine::PointLight>& pLight = CreateDrawable<Engine::PointLight>("light", FindLayer(DEFAULT_LAYER));

	if (!pLight)
	{
		assert(false);
		return false;
	}

	const std::shared_ptr<Engine::Transform>& pLightTransform = pLight->GetTransform();

	if (pLightTransform)
	{
		pLightTransform->SetRX(-PI / 2.f);
		pLightTransform->SetZ(-25.f);
		pLightTransform->SetY(100.f);

		pLightTransform->SetScale({ 5000.f, 5000.f, 5000.f });
	}

	pLight->SetIntensity(1.2f);
	pLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);

	ImguiManager::GetInst()->LoadNavMesh(this, TEXT("navmesh\\nav_test.obj"), MESH_PATH);

	Engine::Scene::CreateProtoType<Player>("Player", Engine::SCENE_TYPE::CURRENT);

	std::shared_ptr<Engine::Drawable> pSponza = Engine::Scene::CreateDrawable<Engine::Drawable>("sponza", FindLayer(DEFAULT_LAYER));

	pSponza->Load(TEXT("Sponza\\sponza.obj"));

	//std::vector<const TCHAR*> vecTexture =
	//{
	//	TEXT("LandScape\\Terrain_Cliff_15_Large.dds"),
	//	TEXT("LandScape\\BD_Terrain_Cliff05.dds"),
	//};

	//std::vector<const TCHAR*> vecNormalTexture =
	//{
	//	TEXT("LandScape\\Terrain_Cliff_15_Large_NRM.bmp"),
	//	TEXT("LandScape\\BD_Terrain_Cliff05_NRM.bmp"),
	//};

	//std::vector<const TCHAR*> vecSpecularTexture =
	//{
	//	TEXT("LandScape\\Terrain_Cliff_15_Large_SPEC.bmp"),
	//	TEXT("LandScape\\BD_Terrain_Cliff05_SPEC.bmp"),
	//};

	//std::vector<const TCHAR*> vecBlendTexture =
	//{
	//	TEXT("LandScape\\baseAlpha.bmp"),
	//	TEXT("LandScape\\RoadAlpha.bmp"),
	//};

	//std::shared_ptr<Engine::Terrain> pTerrain = CreateDrawable<Engine::Terrain>("Terrain", FindLayer(DEFAULT_LAYER));

	//pTerrain->CreateTerrainTexture(vecTexture);
	//pTerrain->CreateTerrainNormalTexture(vecNormalTexture);
	//pTerrain->CreateTerrainSpecularTexture(vecSpecularTexture);
	//pTerrain->CreateBlendTerrainTexture(vecBlendTexture);
	//pTerrain->CreateHeightMap(TEXT("LandScape\\height2.bmp"));

	//std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = pTerrain->FindChild<Engine::ColliderMesh>();

	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pTerrain.get(), &Engine::Terrain::CollisionStay);
	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::LAST, pTerrain.get(), &Engine::Terrain::CollisionEnd);

	//std::shared_ptr<Engine::Particle> pParticle = CreateDrawable<Engine::Particle>("particle", FindLayer(ALPHA_LAYER), 1024);

	//pParticle->SetEmitTime(1.0f);
	//pParticle->SetStartColor(Engine::White);
	//pParticle->SetEndColor(Engine::Blue);
	//pParticle->SetStartSize({ 1.f, 1.f });
	//pParticle->SetEndSize({ 1.f, 1.f });
	//pParticle->SetMaxLifeTime(5.f);
	//pParticle->SetVelocity(Engine::Vector3(0.f, 1.f, 0.f));
	//pParticle->SetMinCreatePosition(Engine::Vector3(-10.f, -10.f, -10.f));
	//pParticle->SetMaxCreatePosition(Engine::Vector3(10.f, 10.f, 10.f));
	//pParticle->CreateBindable<Engine::Texture>("ParticleTexture", TEXT("Particle\\Snow50px.png"), TEXTURE_PATH, 0);
	//pParticle->SetMaxFrame(16);
	//pParticle->SetFrameWidth(4);
	//pParticle->SetFrameHeight(4);

	std::vector<std::wstring> vecDiffuse = { 
		TEXT("Decal\\Decal.png") ,
		TEXT("Decal\\Decal1.png") ,
		TEXT("Decal\\free-blood-texture_COLOR.png") ,
		TEXT("Decal\\Shout24674-perfil3_COLOR.png") ,
	};
	std::vector<std::wstring> vecNormal = { 
		TEXT("Decal\\Decal_NRM.png") ,
		TEXT("Decal\\Decal1_NRM.png") ,
		TEXT("Decal\\free-blood-texture_NRM.png") ,
		TEXT("Decal\\Shout24674-perfil3_NRM.png") ,
	};
	std::vector<std::wstring> vecSpec = { 
		TEXT("Decal\\Decal_SPEC.png") ,
		TEXT("Decal\\Decal1_SPEC.png") ,
		TEXT("Decal\\free-blood-texture_SPEC.png") ,
		TEXT("Decal\\Shout24674-perfil3_SPEC.png") ,
	};

	for (int i = 0; i < 4; ++i)
	{
		std::shared_ptr<Engine::Decal> pDecal = CreateDrawable<Engine::Decal>("decal", FindLayer(DEFAULT_LAYER));

		pDecal->CreateBindable<Engine::Texture>("DecalDiffuse", vecDiffuse[i].c_str(), TEXTURE_PATH);
		pDecal->CreateBindable<Engine::Texture>("DecalNormal", vecNormal[i].c_str(), TEXTURE_PATH, 1);
		pDecal->CreateBindable<Engine::Texture>("DecalSpecular", vecSpec[i].c_str(), TEXTURE_PATH, 2);

		std::shared_ptr<Engine::Mesh> pBoxMesh = pDecal->CreateBindable<Engine::Mesh>("Box", Engine::Box::CreateTextureVertex<Engine::VertexStandard>(), Engine::Box::GetTextureIndex());
		pDecal->FindAndAddBind<Engine::Topology>("TriangleList");

		std::shared_ptr<Engine::Transform> pDecalTransform = pDecal->GetTransform();

		if (pDecalTransform)
		{
			if (i == 0)
			{
				pDecalTransform->SetScale(10.f, 10.f, 10.f);
			}
			else
			{
				pDecalTransform->SetScale(50.f, 50.f, 50.f);
			}

			pDecalTransform->SetPosition(60.f * i, 0.f, 0.f);
		}

		pDecal->SetMaxFadeTime(20.f);

		pDecal->SetFadeStartTime(15.f);
	}

	//std::shared_ptr<Engine::Drawable> pBox = CreateDrawable<Engine::Drawable>("box", FindLayer(DEFAULT_LAYER));

	//pBox->CreateBindable<Engine::Texture>("DecalDiffuse", TEXT("Decal\\Decal.png"), TEXTURE_PATH);
	//pBox->CreateBindable<Engine::Texture>("DecalNormal", TEXT("Decal\\Decal_NRM.png"), TEXTURE_PATH, 1);
	//pBox->CreateBindable<Engine::Texture>("DecalSpecular", TEXT("Decal\\Decal_SPEC.png"), TEXTURE_PATH, 2); 
	//pBox->FindAndAddBind<Engine::Topology>("TriangleList");
	//pBox->FindAndAddBind<Engine::InputLayout>(STANDARD_INPUT_LAYOUT);
	//pBox->FindAndAddBind<Engine::VertexShader>(STANDARD_VS);
	//pBox->FindAndAddBind<Engine::PixelShader>(STANDARD_PS);
	//pBox->AddChild(std::static_pointer_cast<Engine::Bindable>(pBoxMesh));

	//std::shared_ptr<Engine::Transform> pBoxTransform = pBox->GetTransform();

	//pBoxTransform->SetScale(50.f, 50.f, 50.f);
	//pBoxTransform->SetPosition(50.f, 0.f, 0.f);

	//pBox->Disable();

	return true;
}

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
#include "Bindable/Fluid.h"
#include "Bindable/SkyBox.h"
#include "Render/RenderManager.h"
#include "Bindable/Cloth.h"
#include "Bindable/Sphere.h"
#include "Bindable/ColliderSphere.h"
#include "Bindable/NavMesh.h"

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
	tLightOrthoInfo.fRight = 100.f;
	tLightOrthoInfo.fTop = 100.f;
	tLightOrthoInfo.fBottom = 0.f;

	pDirLight->SetOrthoInfo(tLightOrthoInfo);

	pDirLight->SetIntensity(1.f);

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

	pLight->SetIntensity(0.6f);
	pLight->SetLightType(Engine::LIGHT_TYPE::DIRECTIONAL);

	//ImguiManager::GetInst()->LoadNavMesh(this, TEXT("navmesh\\nav_test.obj"), MESH_PATH);

	Engine::Scene::CreateProtoType<Player>("Player", Engine::SCENE_TYPE::CURRENT);

	//std::shared_ptr<Engine::Drawable> pSponza = Engine::Scene::CreateDrawable<Engine::Drawable>("sponza", FindLayer(DEFAULT_LAYER));

	//pSponza->Load(TEXT("Sponza\\sponza.obj"));

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

	std::shared_ptr<Engine::Terrain> pTerrain = CreateDrawable<Engine::Terrain>("Terrain", FindLayer(DEFAULT_LAYER));

	pTerrain->CreateTerrainTexture(vecTexture);
	pTerrain->CreateTerrainNormalTexture(vecNormalTexture);
	pTerrain->CreateTerrainSpecularTexture(vecSpecularTexture);
	pTerrain->CreateBlendTerrainTexture(vecBlendTexture);
	pTerrain->CreateHeightMap(TEXT("LandScape\\height2.bmp"));

	std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = pTerrain->FindChild<Engine::ColliderMesh>();

	pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, ImguiManager::GetInst(), &ImguiManager::CollisionStay);

	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pTerrain.get(), &Engine::Terrain::CollisionStay);
	//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::LAST, pTerrain.get(), &Engine::Terrain::CollisionEnd);

	std::vector<float> vecPoints;

	pTerrain->GetPoints(vecPoints);

	std::vector<int> vecTris;

	pTerrain->GetTris(vecTris);

	Engine::Vector3 vMin = {FLT_MAX,FLT_MAX, FLT_MAX};
	Engine::Vector3 vMax = { FLT_MIN, FLT_MIN, FLT_MIN};

	for (int i = 0; i < static_cast<int>(vecPoints.size()) / 3; ++i)
	{
		if (vMin.x > vecPoints[i * 3])
		{
			vMin.x = vecPoints[i * 3];
		}

		if (vMin.y > vecPoints[i * 3 + 1])
		{
			vMin.y = vecPoints[i * 3 + 1];
		}

		if (vMin.z > vecPoints[i * 3 + 2])
		{
			vMin.z = vecPoints[i * 3 + 2];
		}

		if (vMax.x < vecPoints[i * 3])
		{
			vMax.x = vecPoints[i * 3];
		}

		if (vMax.y < vecPoints[i * 3 + 1])
		{
			vMax.y = vecPoints[i * 3 + 1];
		}

		if (vMax.z < vecPoints[i * 3 + 2])
		{
			vMax.z = vecPoints[i * 3 + 2];
		}
	}

	std::shared_ptr<Engine::NavMesh> pTerrainNavMesh = ImguiManager::GetInst()->CreateNavMesh(vecPoints, vecTris, vMax, vMin);

	pTerrain->AddChild(pTerrainNavMesh);

	Engine::RenderManager::GetInst()->SetSkyBox(CreateDrawable<Engine::SkyBox>("SkyBox", FindLayer(DEFAULT_LAYER), TEXT("gnbRv.jpg")));

	return true;
}

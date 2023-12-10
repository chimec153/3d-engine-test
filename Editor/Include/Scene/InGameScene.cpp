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
#include "Input/Input.h"
namespace Editor
{
	InGameScene::InGameScene()
	{

	}

	bool InGameScene::Init()
	{
		__super::Init();

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

		CreateProtoType<Player>("Player", Engine::SCENE_TYPE::CURRENT);

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

		pTerrain->CreateTerrainTexture("TerrainDiffuse", vecTexture);
		pTerrain->CreateTerrainNormalTexture("TerrainNormal", vecNormalTexture);
		pTerrain->CreateTerrainSpecularTexture("TerrainSpecular", vecSpecularTexture);
		pTerrain->CreateBlendTerrainTexture("TerrainBlend", vecBlendTexture);
		pTerrain->CreateHeightMap("TerrainHeight", TEXT("LandScape\\height2.bmp"));

		std::shared_ptr<Engine::ColliderMesh> pTerrainCollider = pTerrain->FindChild<Engine::ColliderMesh>();

		pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, ImguiManager::GetInst(), &ImguiManager::CollisionStay);

		//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::STAY, pTerrain.get(), &Engine::Terrain::CollisionStay);
		//pTerrainCollider->SetCallBack(Engine::COLLISION_TYPE::LAST, pTerrain.get(), &Engine::Terrain::CollisionEnd);

		std::vector<float> vecPoints;

		pTerrain->GetPoints(vecPoints);

		std::vector<int> vecTris;

		pTerrain->GetTris(vecTris);

		Engine::Vector3 vMin = { FLT_MAX,FLT_MAX, FLT_MAX };
		Engine::Vector3 vMax = { FLT_MIN, FLT_MIN, FLT_MIN };

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
}
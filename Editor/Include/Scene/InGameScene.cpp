#include "InGameScene.h"
#include "../Imgui/ImguiManager.h"
#include "Bindable/Camera.h"
#include "Bindable/PointLight.h"
#include "Bindable/TransformBuffer.h"
#include "Scene/Scene.h"
#include "../Object/Player.h"

InGameScene::InGameScene()
{
	
}

bool InGameScene::Init()
{
	const std::shared_ptr<Engine::Camera>& pCamera = CreateDrawable<Engine::Camera>("camera", FindLayer(DEFAULT_LAYER));

	Engine::Graphics::GetInst()->SetCamera(pCamera);

	const std::shared_ptr<Engine::PointLight>& pDirLight = CreateDrawable<Engine::PointLight>("light2", FindLayer(DEFAULT_LAYER));

	std::shared_ptr<Engine::TransformBuffer> pDirLightTransform = pDirLight->GetTransform();

	pDirLightTransform->SetRX(0.962f);
	pDirLightTransform->SetX(15.f);
	pDirLightTransform->SetY(50.f);
	pDirLightTransform->SetZ(-20.f);

	Engine::ORTHOINFO tLightOrthoInfo = pDirLight->GetOrthoInfo();

	tLightOrthoInfo.fLeft = -50.f;
	tLightOrthoInfo.fRight = 50.f;
	tLightOrthoInfo.fTop = 50.f;
	tLightOrthoInfo.fBottom = -50.f;

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

	const std::shared_ptr<Engine::TransformBuffer>& pLightTransform = pLight->GetTransform();

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

	return true;
}

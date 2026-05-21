#include "StartScene.h"
#include "Core/ObjectFactory.h"
REGISTER_SCENE(Client::StartScene, StartScene)
#include "Bindable/Camera.h"
#include "Core/Graphics.h"

namespace Client
{
	StartScene::StartScene()
	{
	}

	StartScene::~StartScene()
	{
	}

	bool StartScene::Init()
	{
		std::shared_ptr<Engine::Camera> pCamera;
		if (auto pCameraObj = CreateGameObject("camera", FindLayer(DEFAULT_LAYER)))
		{
			pCamera = pCameraObj->AddComponent<Engine::Camera>("camera");
		}

		Engine::Graphics::GetInst()->SetCamera(pCamera);

		return true;
	}
}
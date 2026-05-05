#include "StartScene.h"
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
		std::shared_ptr<Engine::Camera> pCamera = CreateComponent<Engine::Camera>("camera", FindLayer(DEFAULT_LAYER));

		Engine::Graphics::GetInst()->SetCamera(pCamera);

		return true;
	}
}
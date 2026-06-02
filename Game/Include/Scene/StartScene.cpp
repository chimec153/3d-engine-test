#include "StartScene.h"
#include "Core/ObjectFactory.h"
REGISTER_SCENE(Client::StartScene, StartScene)
#include "GameScene.h"
#include "WeaponComboScene.h"
#include "../UI/StartMenu.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Core/Window.h"
#include "Scene/SceneManager.h"
#include <vector>

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
		if (!FindLayer(DEFAULT_LAYER)) AddLayer(DEFAULT_LAYER);

		std::shared_ptr<Engine::Camera> pCamera;
		if (auto pCameraObj = CreateGameObject("camera", FindLayer(DEFAULT_LAYER)))
		{
			pCamera = pCameraObj->AddComponent<Engine::Camera>("camera");
		}

		Engine::Graphics::GetInst()->SetCamera(pCamera);

		// Start menu: three stacked buttons.
		//   1. Start Game     -> swap to GameScene (NEXT, deferred ChangeScene).
		//   2. Weapon Combine -> placeholder WeaponComboScene.
		//   3. Quit           -> end the Run loop (Window::StopRunning).
		std::vector<MenuItem> items = {
			{ L"Start Game",     0x2E7D32,
			  [] { Engine::SceneManager::GetInst()->CreateScene<Client::GameScene>(); } },
#ifdef _DEBUG
			{ L"Weapon Combine", 0x1565C0,
			  [] { Engine::SceneManager::GetInst()->CreateScene<Client::WeaponComboScene>(); } },
#endif
			{ L"Quit",           0xB71C1C,
			  [] { Engine::Window::GetInst()->StopRunning(); } },
		};

		if (auto pMenuObj = CreateGameObject("start_menu", FindLayer(DEFAULT_LAYER)))
		{
			pMenuObj->AddComponent<StartMenu>("menu", items);
		}

		return true;
	}
}

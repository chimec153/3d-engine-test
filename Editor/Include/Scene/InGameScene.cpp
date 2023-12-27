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
#include "Bindable/ColliderOBB.h"
#include "Bindable/Animation.h"
#include "Bindable/Material.h"
#include "Bindable/BindableManager.h"

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

		std::shared_ptr<Engine::Drawable> pTest = CreateDrawable<Engine::Drawable>("test", FindLayer(DEFAULT_LAYER));

		pTest->Load(TEXT("Attack_A_Slow.FBX"));

		Engine::RenderManager::GetInst()->SetSkyBox(CreateDrawable<Engine::SkyBox>("SkyBox", FindLayer(DEFAULT_LAYER), TEXT("TYbvO.jpg")));
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
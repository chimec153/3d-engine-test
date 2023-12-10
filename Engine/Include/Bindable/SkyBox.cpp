#include "SkyBox.h"
#include "TransformBuffer.h"
#include "Camera.h"
#include "../Render/RenderManager.h"
#include "Texture.h"

Engine::SkyBox::SkyBox()	:
	Drawable()
{
	SetBindableType(BINDABLE_TYPE::SKYBOX);
}

Engine::SkyBox::SkyBox(const TCHAR* pTexturePath, const std::string& strKey)
{
	SetBindableType(BINDABLE_TYPE::SKYBOX);

	FindAndAddBind<Mesh>("Box");
	FindAndAddBind<VertexShader>("EnvironmentVS");
	FindAndAddBind<PixelShader>("EnvironmentPS");
	FindAndAddBind<class Topology>(STANDARD_TOPOLOGY);
	FindAndAddBind<class InputLayout>(STANDARD_INPUT_LAYOUT);

	CreateBindable<Texture>("SkyBoxTexture", pTexturePath, strKey, 5);
}

bool Engine::SkyBox::Init()
{
	if (!__super::Init())
	{
		return false;
	}

	GetTransform()->SetScale(5000.f, 5000.f, 5000.f);

	RenderManager::GetInst()->SetSkyBox(std::static_pointer_cast<SkyBox>(shared_from_this()));

	return true;
}

void Engine::SkyBox::Update(float fDeltaTime)
{
	std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

	if (pCamera)
	{
		GetTransform()->SetPosition(pCamera->GetTransform()->GetPosition());
	}

	__super::Update(fDeltaTime);
}

void Engine::SkyBox::PreDraw(float fDeltaTime)
{
}

void Engine::SkyBox::Save(FILE* pFile)
{
	__super::Save(pFile);
}

void Engine::SkyBox::Load(FILE* pFile)
{
	__super::Load(pFile);

	RenderManager::GetInst()->SetSkyBox(std::static_pointer_cast<SkyBox>(shared_from_this()));
}

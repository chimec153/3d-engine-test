#include "SkyBox.h"
#include "TransformBuffer.h"
#include "Camera.h"
#include "../Render/RenderManager.h"

Engine::SkyBox::SkyBox(const TCHAR* pTexturePath, const std::string& strKey)
{
	FindAndAddBind<Mesh>("Box");
	FindAndAddBind<VertexShader>("EnvironmentVS");
	FindAndAddBind<PixelShader>("EnvironmentPS");
	FindAndAddBind<class Topology>(STANDARD_TOPOLOGY);
	FindAndAddBind<class InputLayout>(STANDARD_INPUT_LAYOUT);

	CreateBindable<Texture>("SkyBoxTexture", pTexturePath, strKey);

	GetTransform()->SetScale(5000.f, 5000.f, 5000.f);
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

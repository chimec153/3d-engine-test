#include "Client.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"
#include "Bindable/BindableRegistry.h"
#include "Render/RenderManager.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderManager.h"
#include "Collision/CollisionManager.h"
#include "Input/Input.h"
#include "Thread/ThreadManager.h"
#include "Core/Graphics.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    if (!Engine::Window::GetInst()->Init(TEXT("tycoon"), TEXT("test"), hInstance, Engine::Window::WndProc))
    {
        Engine::Window::DestroyInst();
        return -1;
    }

    Engine::SceneManager::GetInst()->CreateScene<Client::GameScene>();

    Engine::Window::GetInst()->Run();

    // Shutdown order: release D3D-holding objects before the device.
    Engine::SceneManager::DestroyInst();
    Engine::RenderManager::DestroyInst();
    Engine::ResourceManager::DestroyInst();
    Engine::ShaderManager::DestroyInst();
    Engine::CollisionManager::DestroyInst();
    Engine::CInput::DestroyInst();
    Engine::ThreadManager::DestroyInst();
    Engine::BindableRegistry::DestroyAll();
    Engine::Graphics::DestroyInst();
    Engine::Window::DestroyInst();

	return 0;
}
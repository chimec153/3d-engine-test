#include "Client.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"

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

    // Shutdown is centralised in Window::~Window — it tears down every
    // engine-side manager (Scene, Render, Resource, Shader, Bindable
    // caches, Graphics, etc.) in the correct order.
    Engine::Window::DestroyInst();

	return 0;
}
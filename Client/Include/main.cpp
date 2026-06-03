#include "Client.h"
#include "Scene/SceneManager.h"
#include "Scene/GameScene.h"
#include "Scene/StartScene.h"
#include "Util/Telemetry.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    if (!Engine::Window::GetInst()->Init(TEXT("cube trouble"), TEXT("cube trouble"), hInstance, Engine::Window::WndProc))
    {
        Engine::Window::DestroyInst();
        return -1;
    }

#ifndef _DEBUG
    Client::Telemetry::GetInst().Init();
#endif

    Engine::SceneManager::GetInst()->CreateScene<Client::StartScene>();

    Engine::Window::GetInst()->Run();

    // Shutdown is centralised in Window::~Window - it tears down every
    // engine-side manager (Scene, Render, Resource, Shader, Bindable
    // caches, Graphics, etc.) in the correct order.
    Engine::Window::DestroyInst();

    // Telemetry shutdown AFTER DestroyInst: GameScene's destructor enqueues the
    // final run_end("quit") during DestroyInst, and Shutdown() flushes the queue
    // (and joins the worker) before the process exits.
#ifndef _DEBUG
    Client::Telemetry::GetInst().Shutdown();
#endif

    return 0;
}

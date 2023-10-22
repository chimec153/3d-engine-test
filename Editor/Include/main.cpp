#define _CRTDBG_MAP_ALLOC

#include "Editor.h"
#include "Window.h"
#include "Imgui/ImguiManager.h"
#include "Core/Graphics.h"
#include "Scene/SceneManager.h"
#include "Scene/InGameScene.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);

    if (!Engine::Window::GetInst()->Init(TEXT("test"), TEXT("test2"), hInstance, Window::WndProc))
    {
        Engine::Window::DestroyInst();
        return -1;
    }

    // Imgui ÃÊ±âÈ­
    if (!ImguiManager::GetInst()->Init(Engine::Window::GetInst()->GetWinHandle()))
    {
        ImguiManager::DestroyInst();
        Engine::Window::DestroyInst();
        return -1;
    }

    Engine::SceneManager::GetInst()->CreateScene<InGameScene>();

    Window w;

    int iRetVal = w.Run();

    ImguiManager::DestroyInst();

    Engine::Window::DestroyInst();

	return iRetVal;
}
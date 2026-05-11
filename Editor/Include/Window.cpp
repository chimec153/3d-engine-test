#include "Window.h"
#include "Core/Window.h"
#include "Imgui/ImguiManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Render/RenderManager.h"
#include "Input/Input.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Window::Window()
{

}

bool Window::Init()
{
	return true;
}
int Window::Run()
{
	MSG msg;

	while (Engine::Window::GetInst()->IsRun())
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Logic();
		}
	}

	return static_cast<int>(msg.wParam);
}


void Window::Logic()
{
	Editor::ImguiManager::GetInst()->Update(0.f);

	Engine::Scene* pScene = Engine::SceneManager::GetInst()->GetScene();

	if (pScene)
	{
		static char strLayer[MAX_PATH] = DEFAULT_LAYER;

		ImGui::InputText("Layer", strLayer, MAX_PATH);

		Editor::ImguiManager::GetInst()->Layer_DrawListImgui(pScene->FindLayer(strLayer));
	}

	Editor::ImguiManager::GetInst()->MRT_ShowImGuiImage(Engine::RenderManager::GetInst()->GetMRT());
	Editor::ImguiManager::GetInst()->MRT_ShowImGuiImage(Engine::RenderManager::GetInst()->GetDepthBuffer(Engine::LIGHT_TYPE::DIRECTIONAL));

	Editor::ImguiManager::GetInst()->MRT_ShowImGuiImage(Engine::RenderManager::GetInst()->GetDecalMRT(), "DecalMRT");

	Engine::Window::GetInst()->Logic();
}

LRESULT __stdcall Window::WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, iMsg, wParam, lParam))
		return true;

	switch (iMsg)
	{
	case WM_MOUSEMOVE:
	{
		POINT pt;

		GetCursorPos(&pt);

		ScreenToClient(hWnd, &pt);

		bool bIn = pt.x >= 0 && pt.x <= Engine::Window::GetInst()->GetWidth() &&
			pt.y >= 0 && pt.y <= Engine::Window::GetInst()->GetHeight();

		if (bIn == Engine::Window::GetInst()->IsCursorEnabled() && !Engine::Window::GetInst()->IsLockRotation())
		{
			Engine::Window::GetInst()->CursorDisable();
		}
	}
	break;
	case WM_MOUSELEAVE:
		Engine::Window::GetInst()->CursorEnable();
		break;
	case WM_ACTIVATE:
		if (wParam)
		{
			Engine::CInput::GetInst()->Enable();
		}
		else
		{
			Engine::CInput::GetInst()->Disable();
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		Engine::Window::GetInst()->StopRunning();
		return wParam;
	default:
		return DefWindowProc(hWnd, iMsg, wParam, lParam);
	}
	return 0;
}
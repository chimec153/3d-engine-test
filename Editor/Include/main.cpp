#define _CRTDBG_MAP_ALLOC

#include "Editor.h"
#include "Window.h"
#include "Imgui/ImguiManager.h"
#include "Core/Graphics.h"
#include "Scene/SceneManager.h"
#include "Scene/InGameScene.h"
#include "Bindable/BindableRegistry.h"
#include "Render/RenderManager.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderManager.h"
#include "Collision/CollisionManager.h"
#include "Input/Input.h"
#include "Thread/ThreadManager.h"

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

    // Imgui �ʱ�ȭ
    if (!Editor::ImguiManager::GetInst()->Init(Engine::Window::GetInst()->GetWinHandle()))
    {
        Editor::ImguiManager::DestroyInst();
        Engine::Window::DestroyInst();
        return -1;
    }

    Engine::Window::GetInst()->SetPrePresentCallback([]()
    {
        Editor::ImguiManager::GetInst()->Render(0.f);
    });

    Engine::SceneManager::GetInst()->CreateScene<Editor::InGameScene>();

    Window w;

    int iRetVal = w.Run();

    // Shutdown order matters: release things that hold D3D resources
    // (scene → editor UI → rendering pipeline → cached Bindables) BEFORE
    // the device itself goes away. Without this, manager singletons
    // outlive Graphics and every cached shader/buffer/texture shows up
    // as a "live object" warning.
    Engine::SceneManager::DestroyInst();      // Scenes → Layers → GameObjects → Components
    Editor::ImguiManager::DestroyInst();      // Editor D3D resources + ImGui DX11 backend
    Engine::RenderManager::DestroyInst();     // HDR/Bloom/MRT pipeline resources
    Engine::ResourceManager::DestroyInst();   // Skeletons, sequences, etc.
    Engine::ShaderManager::DestroyInst();     // Shader cache (if any extra refs)
    Engine::CollisionManager::DestroyInst();  // Collision-side caches
    Engine::CInput::DestroyInst();            // Input (DirectInput device, etc.)
    Engine::ThreadManager::DestroyInst();     // Loading threads + any held bindables
    Engine::BindableRegistry::DestroyAll();   // Every BindableManager<T> cache

    // D3D11 live-object reports — two snapshots bracketing Graphics
    // teardown so we can see both:
    //   1) state right before Graphics destroys (peak, includes
    //      swapchain/context/backbuffer that Graphics still owns)
    //   2) state right after Graphics destroys — anything here is a
    //      genuine leak that survived every explicit cleanup
    //
    // ID3D11Debug is the same COM object as the device, so holding a
    // pDebug ref keeps the device alive even after Graphics::DestroyInst
    // releases its own ref. We Release pDebug last, which actually
    // destroys the device.
    // D3D11 live-object snapshot before tearing down the device. With the
    // explicit shutdown sequence above (managers + BindableRegistry +
    // Graphics) this should list only swapchain/context/backbuffer that
    // Graphics owns — those go away in Graphics::DestroyInst on the next
    // line, leaving the device with refcount 0.
    //
    // Note: the reported "Refcount" includes the debug layer's own +1
    // during the report itself, so what looks like a 1-2 ref residual on
    // the device line is normal. The actual external ref after pDebug
    // gets Released is 0 (verified during initial bring-up).
    //{
    //    ID3D11Device* pDev = Engine::Graphics::GetInst()->GetDevice();
    //    ID3D11Debug* pDebug = nullptr;
    //    if (pDev && SUCCEEDED(pDev->QueryInterface(__uuidof(ID3D11Debug),
    //        reinterpret_cast<void**>(&pDebug))))
    //    {
    //        pDebug->ReportLiveDeviceObjects(static_cast<D3D11_RLDO_FLAGS>(
    //            D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL));
    //        pDebug->Release();
    //    }
    //}

    // Release the D3D device and everything Graphics holds (swapchain,
    // context, backbuffer RTV/DSV). Done after the live-object snapshot
    // so the report shows them, then they get freed cleanly.
    Engine::Graphics::DestroyInst();

    Engine::Window::DestroyInst();

	return iRetVal;
}
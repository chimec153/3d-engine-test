#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Core/Graphics.h"
#include "RenderV2/Drawables/Box.h"
#include "RenderV2/Drawables/Quad.h"
#include "RenderV2/Drawables/Sphere.h"
#include "RenderV2/Drawables/Mesh.h"

class CTestScene : public Engine::Scene
{
public:
    virtual bool Init() override
    {
        if (!Engine::Scene::Init())
            return false;

        ID3D11Device* device = Engine::Graphics::GetInst()->GetDevice();

        // Box (textured cube, center)
        auto box = std::make_shared<Engine::RenderV2::Drawables::Box>();
        if (box->Init(device))
            AddV2Drawable(box);

        // Sphere (left)
        auto sphere = std::make_shared<Engine::RenderV2::Drawables::Sphere>(16, 24);
        if (sphere->Init(device))
        {
            sphere->SetPosition({-1.5f, 0.0f, 0.0f});
            AddV2Drawable(sphere);
        }

        // Quad (alpha-blended, slightly in front of cube)
        auto quad = std::make_shared<Engine::RenderV2::Drawables::Quad>();
        if (quad->Init(device))
        {
            quad->SetPosition({0.4f, 0.0f, -0.7f});
            AddV2Drawable(quad);
        }

        // Walking mesh (lit, animated — animation currently upside-down due
        // to FbxLoader matTransform convention; static skinning works).
        auto mesh = std::make_shared<Engine::RenderV2::Drawables::Mesh>();
        if (mesh->Init(device, L"Walking.fbx"))
        {
            mesh->SetPosition({1.5f, -0.5f, 0.0f});
            mesh->SetScale(1.0f);
            AddV2Drawable(mesh);
        }

        return true;
    }
};

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

    Engine::SceneManager::GetInst()->CreateScene<CTestScene>();

    Engine::Window::GetInst()->Run();

    Engine::Window::DestroyInst();

    return 0;
}

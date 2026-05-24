#include "ProjectModule.h"
#include "Core/MountPointRegistry.h"
#include "Scene/SceneManager.h"
#include "Resource/ResourceManager.h"
#include "Bindable/BindableManager.h"
#include "Bindable/Mesh.h"
#include "Bindable/Texture.h"
#include "Bindable/Material.h"
#include "Input/Input.h"
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

namespace Editor
{
    ProjectModule& ProjectModule::Get()
    {
        static ProjectModule inst;
        return inst;
    }

    // Drop the previous world before pulling in a new game's content. We
    // only clear "game content" caches (meshes, textures, materials, and
    // the ResourceManager's sequence/skeleton/sound maps) — engine-essential
    // bindables (shaders, input layouts, topologies, samplers, ...) stay
    // so editor UI (selection outline, gizmo, etc.) keeps rendering.
    //
    // Outstanding shared_ptrs held by an in-flight scene keep their targets
    // alive; only future tag lookups stop seeing them. ClearScene() runs
    // first so the scene drops its refs before we wipe the caches.
    static void ResetWorld()
    {
        Engine::CInput::GetInst()->ClearActions();
        Engine::ResourceManager::GetInst()->Clear();
        //Engine::BindableManager<Engine::Mesh>::GetInst()->Clear();
        //Engine::BindableManager<Engine::Texture>::GetInst()->Clear();
        //Engine::BindableManager<Engine::Material>::GetInst()->Clear();
    }

    bool ProjectModule::Load(const std::wstring& dllPath)
    {
        if (m_hModule) return false;  // already loaded — restart Editor to swap

        ResetWorld();

        HMODULE h = ::LoadLibraryW(dllPath.c_str());
        if (!h) return false;

        m_hModule = h;
        m_path    = dllPath;

        // Mount /Game/ at the DLL's sibling Resource folder. Convention:
        // Game DLL lives next to a Resource/ folder (mirrors Unreal's
        // .uproject + Content/ layout).
        size_t slash = dllPath.find_last_of(L"/\\");
        if (slash != std::wstring::npos)
        {
            std::wstring projectRoot = dllPath.substr(0, slash + 1);
            Engine::MountPointRegistry::Mount("/Game/", projectRoot + L"Resource\\");
        }

        return true;
    }

    std::wstring ProjectModule::OpenDialog(HWND hOwner)
    {
        wchar_t buf[MAX_PATH] = {0};

        OPENFILENAMEW ofn = {};
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hOwner;
        ofn.lpstrFilter  = L"Game module (*.dll)\0*.dll\0All files (*.*)\0*.*\0";
        ofn.lpstrFile    = buf;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrTitle   = L"Open Project Module";
        ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!::GetOpenFileNameW(&ofn)) return {};
        return std::wstring(buf);
    }
}

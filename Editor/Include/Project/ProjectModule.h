#pragma once
#include <Windows.h>
#include <string>

namespace Editor
{
    // Loads a game DLL via LoadLibrary. The DLL's static initialisers fire
    // on load — that's how SceneFactory / GameObjectFactory get populated
    // (see REGISTER_SCENE / REGISTER_GAMEOBJECT in Engine/Core/ObjectFactory.h).
    //
    // No FreeLibrary support yet: hot-reload would leave dangling lambdas
    // in the Engine-side registries.
    class ProjectModule
    {
    public:
        static ProjectModule& Get();

        bool Load(const std::wstring& dllPath);
        bool IsLoaded() const { return m_hModule != nullptr; }
        const std::wstring& Path() const { return m_path; }

        // Opens a Win32 file dialog. Returns selected path or empty on cancel.
        static std::wstring OpenDialog(HWND hOwner);

    private:
        HMODULE      m_hModule = nullptr;
        std::wstring m_path;
    };
}

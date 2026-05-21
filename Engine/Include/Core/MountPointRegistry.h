#pragma once

#include "Macro.h"
#include <string>

namespace Engine
{
    // Unreal-style virtual mount points.
    //
    //   /Engine/Mesh/foo.mesh   -> <ExeDir>/Resource/Mesh/foo.mesh
    //   /Game/Mesh/Hero.mesh    -> <ProjectDir>/Resource/Mesh/Hero.mesh
    //
    // Mounts are registered as "name" (e.g. "/Engine/") + "fsRoot" (e.g.
    // "C:/.../Editor/Bin/Resource/"). Both should end with a slash.
    // Resolve(virtualPath) splits the leading "/<name>/" prefix and appends
    // the rest to fsRoot.
    //
    // Methods are ENGINE_DLL so the registry storage lives in Engine.dll
    // and Game.dll / Editor.exe share a single map.
    class ENGINE_DLL MountPointRegistry
    {
    public:
        // Mount name must look like "/Game/" (leading + trailing slash).
        // Overwrites any previous binding for the same name.
        static void Mount(const std::string& mountName, const std::wstring& fsRoot);
        static void Unmount(const std::string& mountName);

        // True if path starts with a slash (cheap heuristic; full resolution
        // is what actually validates the mount exists).
        static bool IsVirtual(const std::string& path);
        static bool IsVirtual(const std::wstring& path);

        // Returns empty string if path doesn't start with a registered mount
        // or is malformed.
        static std::wstring Resolve(const std::string& virtualPath);
        static std::wstring Resolve(const std::wstring& virtualPath);
        static std::string  ResolveMB(const std::string& virtualPath);
    };
}

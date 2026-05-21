#include "MountPointRegistry.h"
#include <unordered_map>
#include <Windows.h>

namespace Engine
{
    static std::unordered_map<std::string, std::wstring>& MountMap()
    {
        static std::unordered_map<std::string, std::wstring> map;
        return map;
    }

    void MountPointRegistry::Mount(const std::string& mountName, const std::wstring& fsRoot)
    {
        MountMap()[mountName] = fsRoot;
    }

    void MountPointRegistry::Unmount(const std::string& mountName)
    {
        MountMap().erase(mountName);
    }

    bool MountPointRegistry::IsVirtual(const std::string& path)
    {
        return !path.empty() && path[0] == '/';
    }
    bool MountPointRegistry::IsVirtual(const std::wstring& path)
    {
        return !path.empty() && path[0] == L'/';
    }

    // Splits "/Mount/sub/file" into ("/Mount/", "sub/file"). Returns false if
    // there's no second slash.
    static bool SplitMount(const std::string& virtualPath, std::string& outMount, std::string& outRest)
    {
        if (virtualPath.size() < 2 || virtualPath[0] != '/') return false;
        size_t second = virtualPath.find('/', 1);
        if (second == std::string::npos) return false;
        outMount = virtualPath.substr(0, second + 1);
        outRest  = virtualPath.substr(second + 1);
        return true;
    }

    std::wstring MountPointRegistry::Resolve(const std::string& virtualPath)
    {
        std::string mount, rest;
        if (!SplitMount(virtualPath, mount, rest))
        {
#ifdef _DEBUG
            std::string msg = "[MountPointRegistry] Malformed virtual path: \"" + virtualPath + "\"\n";
            OutputDebugStringA(msg.c_str());
#endif
            return {};
        }

        auto it = MountMap().find(mount);
        if (it == MountMap().end())
        {
#ifdef _DEBUG
            std::string msg = "[MountPointRegistry] Mount \"" + mount + "\" not registered (requested: \"" + virtualPath + "\")\n";
            OutputDebugStringA(msg.c_str());
#endif
            return {};
        }

        // widen the rest (file paths are ASCII-only in this project).
        std::wstring wrest(rest.begin(), rest.end());
        return it->second + wrest;
    }

    std::wstring MountPointRegistry::Resolve(const std::wstring& virtualPath)
    {
        // narrow then resolve — fine since mount keys are ASCII-only.
        std::string narrow(virtualPath.begin(), virtualPath.end());
        return Resolve(narrow);
    }

    std::string MountPointRegistry::ResolveMB(const std::string& virtualPath)
    {
        std::wstring w = Resolve(virtualPath);
        if (w.empty()) return {};
        // Narrow back; ASCII-only assumption.
        return std::string(w.begin(), w.end());
    }
}

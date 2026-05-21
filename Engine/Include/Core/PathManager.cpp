#include "PathManager.h"
#include "MountPointRegistry.h"
#include <io.h>

namespace Engine
{
#ifdef _DEBUG
    // Read-only existence probe. _waccess(path, 0) returns 0 iff the file
    // exists. We only warn (no throw / assert) so debugging an asset gap is
    // a one-line log lookup, not a halted process.
    static void WarnIfMissingW(const TCHAR* fullPath, const TCHAR* original)
    {
        if (_waccess(fullPath, 0) != 0)
        {
            wchar_t buf[1024];
            swprintf_s(buf, L"[PathManager] Asset not found: %s  (input: %s)\n",
                       fullPath, original ? original : L"");
            OutputDebugStringW(buf);
        }
    }
    static void WarnIfMissingA(const char* fullPath, const char* original)
    {
        if (_access(fullPath, 0) != 0)
        {
            char buf[1024];
            sprintf_s(buf, "[PathManager] Asset not found: %s  (input: %s)\n",
                      fullPath, original ? original : "");
            OutputDebugStringA(buf);
        }
    }
#endif
    CPathManager* CPathManager::m_pInst = nullptr;

    CPathManager::CPathManager()
    {
    }

    CPathManager::~CPathManager()
    {
        Safe_Delete_Array_Map(m_mapMultibytePath);
        Safe_Delete_Array_Map(m_mapPath);
    }

    bool CPathManager::Init()
    {
        TCHAR* pPath = dbg_new TCHAR[MAX_PATH];

        GetModuleFileName(nullptr, pPath, MAX_PATH);

        int iLength = static_cast<int>(wcslen(pPath));

        for (int i = iLength - 1; i >= 0; --i)
        {
            if (pPath[i] == '/' || pPath[i] == '\\')
            {
                memset(pPath + i + 1, 0, iLength - i);
                break;
            }
        }

        m_mapPath.insert(std::make_pair(ROOT_PATH, pPath));

        char* pMultiBytePath = dbg_new char[MAX_PATH];

#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, pPath, -1, pMultiBytePath, MAX_PATH, nullptr, nullptr);
#else
        strcpy_s(pMultiBytePath, pNewPath);
#endif

        m_mapMultibytePath.insert(std::make_pair(ROOT_PATH, pMultiBytePath));

        AddPath(SHADER_PATH, TEXT("Resource\\Shader\\"));

        AddPath(TEXTURE_PATH, TEXT("Resource\\Texture\\"));

        AddPath(MESH_PATH, TEXT("Resource\\Mesh\\"));

        AddPath(SOUND_PATH, TEXT("Resource\\Sound\\"));

        AddPath(MATERIAL_PATH, TEXT("Resource\\Material\\"));

        // Mount /Engine/ at the exe-relative Resource folder. /Game/ defaults
        // to the same location so shipped clients (where engine + game assets
        // live in one Resource tree) resolve game-mounted paths out of the
        // box. ProjectModule overwrites /Game/ with the project-specific
        // Resource folder when the editor loads a game DLL.
        MountPointRegistry::Mount("/Engine/", std::wstring(pPath) + L"Resource\\");
        MountPointRegistry::Mount("/Game/",   std::wstring(pPath) + L"Resource\\");

        return true;
    }

    const TCHAR* CPathManager::FindPath(const std::string& strPath) const
    {
        std::unordered_map<std::string, const TCHAR*>::const_iterator iter = m_mapPath.find(strPath);

        if (iter == m_mapPath.end())
        {
            return nullptr;
        }

        return iter->second;
    }

    const char* CPathManager::FindMultibytePath(const std::string& strPath) const
    {
        std::unordered_map<std::string, const char*>::const_iterator iter = m_mapMultibytePath.find(strPath);

        if (iter == m_mapMultibytePath.end())
        {
            return nullptr;
        }

        return iter->second;
    }

    void CPathManager::Resolve(const TCHAR* pFilePath, const std::string& strPathKey, TCHAR out[MAX_PATH]) const
    {
        out[0] = 0;
        if (MountPointRegistry::IsVirtual(std::wstring(pFilePath)))
        {
            std::wstring r = MountPointRegistry::Resolve(std::wstring(pFilePath));
            wcscpy_s(out, MAX_PATH, r.c_str());
        }
        else
        {
            const TCHAR* p = FindPath(strPathKey);
            if (p) wcscpy_s(out, MAX_PATH, p);
            wcscat_s(out, MAX_PATH, pFilePath);
        }
#ifdef _DEBUG
        WarnIfMissingW(out, pFilePath);
#endif
    }

    void CPathManager::ResolveMB(const char* pFilePath, const std::string& strPathKey, char out[MAX_PATH]) const
    {
        out[0] = 0;
        if (MountPointRegistry::IsVirtual(std::string(pFilePath)))
        {
            std::string r = MountPointRegistry::ResolveMB(pFilePath);
            strcpy_s(out, MAX_PATH, r.c_str());
        }
        else
        {
            const char* p = FindMultibytePath(strPathKey);
            if (p) strcpy_s(out, MAX_PATH, p);
            strcat_s(out, MAX_PATH, pFilePath);
        }
#ifdef _DEBUG
        WarnIfMissingA(out, pFilePath);
#endif
    }

    void CPathManager::AddPath(const std::string& strPath, const TCHAR* pPath)
    {
        TCHAR* pNewPath = dbg_new TCHAR[MAX_PATH];

        const TCHAR* pRootPath = FindPath();

        if (pRootPath)
        {
            wcscpy_s(pNewPath, MAX_PATH, pRootPath);
        }

        wcscat_s(pNewPath, MAX_PATH, pPath);

        m_mapPath.insert(std::make_pair(strPath, pNewPath));

        char* pMultiBytePath = dbg_new char[MAX_PATH];

#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, pNewPath, -1, pMultiBytePath, MAX_PATH, nullptr, nullptr);
#else
        strcpy_s(pMultiBytePath, pNewPath);
#endif

        m_mapMultibytePath.insert(std::make_pair(strPath, pMultiBytePath));

    }
}
#include "PathManager.h"

namespace Engine
{
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
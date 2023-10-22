#include "ThreadManager.h"

namespace Engine
{
    ThreadManager* ThreadManager::m_pInst = nullptr;

    ThreadManager::ThreadManager()
    {
    }

    ThreadManager::~ThreadManager()
    {
    }

    std::shared_ptr<Thread> ThreadManager::FindThread(const std::string& strTag) const
    {
        std::unordered_map<std::string, std::shared_ptr<Thread>>::const_iterator iter = m_mapThread.find(strTag);

        if (iter == m_mapThread.end())
        {
            return nullptr;
        }

        return iter->second;
    }
}
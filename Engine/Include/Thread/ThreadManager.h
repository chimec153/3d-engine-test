#pragma once

#include "../Core/Macro.h"

namespace Engine
{
	class ENGINE_DLL ThreadManager
	{
	private:
		ThreadManager();
		~ThreadManager();

	private:
		static ThreadManager* m_pInst;

	public:
		static ThreadManager* GetInst()
		{
			return m_pInst ? m_pInst : m_pInst = dbg_new ThreadManager;
		}

		static void DestroyInst()
		{
			SAFE_DELETE(m_pInst);
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<class Thread>>	m_mapThread;

	public:
		template <typename T>
		std::shared_ptr<T> CreateThread(const std::string& strTag)
		{
			std::shared_ptr<T> pThread = std::make_shared<T>();

			if (!pThread->Init())
			{
				return nullptr;
			}

			m_mapThread.insert(std::make_pair(strTag, pThread));

			return pThread;
		}

		std::shared_ptr<Thread> FindThread(const std::string& strTag)	const;
	};

}
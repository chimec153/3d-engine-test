#pragma once

#include "Macro.h"

namespace Engine
{
	class ENGINE_DLL CPathManager
	{
	private:
		CPathManager();
		~CPathManager();

	private:
		static CPathManager* m_pInst;

	public:
		static CPathManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new CPathManager;
			}

			return m_pInst;
		}
		static void DestoryInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		std::unordered_map<std::string, const TCHAR*> m_mapPath;
		std::unordered_map<std::string, const char*> m_mapMultibytePath;

	public:
		bool Init();
		const TCHAR* FindPath(const std::string& strPath = ROOT_PATH)	const;
		const char* FindMultibytePath(const std::string& strPath = ROOT_PATH)	const;
		void AddPath(const std::string& strPath, const TCHAR* pPath);

	};

}
#include "Ref.h"
#include "PathManager.h"

namespace Engine
{
	void CRef::SaveFromFullPath(const TCHAR* pFullPath)
	{
		char strFullPath[MAX_PATH] = {};

#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, pFullPath, -1, strFullPath, static_cast<int>(wcslen(pFullPath)), nullptr, nullptr);
#else
		strcpy_s(strFullPath, pFullPath);
#endif

		SaveFromFullPath(strFullPath);
	}
	void CRef::SaveFromFullPath(const char* pFullPath)
	{
		FILE* pFile = nullptr;

		fopen_s(&pFile, pFullPath, "wb");

		if (pFile)
		{
			Save(pFile);

			fclose(pFile);
		}
	}

	void CRef::SaveFromPath(const char* pFilePath, const std::string& strPathKey)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		SaveFromFullPath(strFullPath);
	}

	void CRef::LoadFromPath(const char* pFilePath, const std::string& strPathKey)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFullPath, "rb");

		if (pFile)
		{
			Load(pFile);

			fclose(pFile);
		}
	}

	void CRef::Save(FILE* pFile)
	{
		int iLength = static_cast<int>(m_strTag.length());

		fwrite(&iLength, 4, 1, pFile);

		if (iLength)
		{
			fwrite(m_strTag.c_str(), 1, iLength, pFile);
		}
	}

	void CRef::Load(FILE* pFile)
	{
		int iLength;

		fread(&iLength, 4, 1, pFile);

		if (iLength)
		{
			std::unique_ptr<char[]> strTag = std::make_unique<char[]>(iLength + 1);

			fread(strTag.get(), 1, iLength, pFile);

			strTag[iLength] = 0;

			m_strTag = strTag.get();
		}
	}
}
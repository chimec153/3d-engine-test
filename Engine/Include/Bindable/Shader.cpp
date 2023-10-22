#include "Shader.h"
#include "../Core/PathManager.h"
#include "../Core/Window.h"

namespace Engine
{
	Shader::Shader(const TCHAR* pFilePath, const char* pEntry) :
		Bindable()
		, pBlob(nullptr)
	{
		int iLength = static_cast<int>(strlen(pEntry));

		m_pEntry = std::make_unique<char[]>(iLength + 1);

		strcpy_s(m_pEntry.get(), iLength + 1, pEntry);

		m_pFullPath = std::make_unique<TCHAR[]>(MAX_PATH);

		const TCHAR* pPath = CPathManager::GetInst()->FindPath("ShaderPath");

		if (pPath)
		{
			wcscpy_s(m_pFullPath.get(), MAX_PATH, pPath);
		}

		wcscat_s(m_pFullPath.get(), MAX_PATH, pFilePath);
	}

	ID3DBlob* Shader::GetBlob() const
	{
		return *pBlob;
	}

	bool Shader::LoadShaderFile(const char* pTarget)
	{
		CPtr<ID3DBlob> pError = nullptr;

		UINT iFlag = 0;

#ifdef _DEBUG
		iFlag |= D3DCOMPILE_DEBUG;
#endif

		if (FAILED(D3DCompileFromFile(m_pFullPath.get(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, m_pEntry.get(), pTarget, iFlag, 0, &pBlob, &pError)))
		{
			OutputDebugStringA((char*)pError->GetBufferPointer());
			OutputDebugString(L"\n");

			assert(false);
			return false;
		}

		return true;
	}

	const std::unique_ptr<char[]>& Shader::GetEntry() const
	{
		return m_pEntry;
	}
}
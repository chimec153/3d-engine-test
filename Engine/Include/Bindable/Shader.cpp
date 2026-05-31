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

		// Resolve through the /Game/ virtual mount rather than a
		// working-directory-relative "Resource\Shader\" path. The mount is an
		// absolute path to the running module's Resource folder, and in the
		// editor ProjectModule re-points /Game/ at the loaded game's Resource
		// — so game shaders (e.g. EnemyInst.hlsl) are found there instead of
		// the editor's own Resource\Shader (which doesn't ship them) or a
		// stale current directory. Callers pass a bare filename; a path that
		// is already virtual ('/'-prefixed) is resolved as-is.
		if (pFilePath && pFilePath[0] == TEXT('/'))
		{
			CPathManager::GetInst()->Resolve(pFilePath, "ShaderPath", m_pFullPath.get());
		}
		else
		{
			TCHAR szVirtual[MAX_PATH] = {};
			wcscpy_s(szVirtual, MAX_PATH, TEXT("/Game/Shader/"));
			wcscat_s(szVirtual, MAX_PATH, pFilePath);
			CPathManager::GetInst()->Resolve(szVirtual, "ShaderPath", m_pFullPath.get());
		}
	}

	ID3DBlob* Shader::GetBlob() const
	{
		return *pBlob;
	}

	bool Shader::LoadShaderFile(const char* pTarget, std::string* pOutError)
	{
		CPtr<ID3DBlob> pError = nullptr;

		UINT iFlag = 0;

#ifdef _DEBUG
		iFlag |= D3DCOMPILE_DEBUG;
#endif

		if (FAILED(D3DCompileFromFile(m_pFullPath.get(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, m_pEntry.get(), pTarget, iFlag, 0, &pBlob, &pError)))
		{
			// D3DCompileFromFile may fail with no error blob (e.g. file missing),
			// so guard the dereference.
			const char* pMsg = pError ? static_cast<const char*>(pError->GetBufferPointer())
			                          : "shader compile failed (no error blob)";
			OutputDebugStringA(pMsg);
			OutputDebugString(L"\n");

			if (pOutError) { *pOutError = pMsg; return false; }   // hot-reload: report, don't halt

			assert(false);
			return false;
		}

		return true;
	}

	bool Shader::Recompile(std::string& outError)
	{
		const char* pTarget = GetTarget();
		if (!pTarget || pTarget[0] == 0)
		{
			outError = "not hot-reloadable";
			return false;
		}
		// Compile without asserting; on failure pBlob is empty but the existing
		// native shader object (held by the subclass) is untouched, so the old
		// shader keeps rendering.
		if (!LoadShaderFile(pTarget, &outError))
			return false;
		CreateFromBlob();
		return true;
	}

	const std::unique_ptr<char[]>& Shader::GetEntry() const
	{
		return m_pEntry;
	}
}
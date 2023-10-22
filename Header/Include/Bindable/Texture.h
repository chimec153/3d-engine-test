#pragma once
#include "Bindable.h"

namespace Engine
{
	class ENGINE_DLL Texture :
		public Bindable
	{
	public:
		Texture();
		Texture(int iCount, int iSize, int iSlot, void* pData);
		Texture(const TCHAR* pFileName, int iSlot = 0);
		Texture(const char* pFileName, int iSlot = 0);
		Texture(const TCHAR* pFileName, const std::string& strPathKey, int iSlot = 0, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		Texture(const char* pFileName, const std::string& strPathKey, int iSlot = 0);
		Texture(const std::vector<const TCHAR*>& pFileName, const std::string& strPathKey, int iSlot = 0);
		virtual ~Texture() override;

	private:
		CPtr<ID3D11ShaderResourceView>	m_pSRV;
		int	m_iSlot;
		TCHAR m_strFullPath[MAX_PATH];
		CPtr<ID3D11Texture2D>	m_pTexture;

	public:
		int GetSlot()	const;
		bool LoadTexture(const TCHAR* pFileName, DirectX::ScratchImage& image);
		bool LoadTextureFromFullPath(const TCHAR* pFileName, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		bool LoadTextureFromFullPath(const std::vector<const TCHAR*>& pFileName);
		bool CreateShaderResourceView(const DirectX::ScratchImage& image, D3D_SRV_DIMENSION eDimension = D3D11_SRV_DIMENSION_TEXTURE2D, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		bool CreateTexture(int iCount, int iSize, int iSplice, void* pData);
		CPtr<ID3D11ShaderResourceView> GetSRV()	const;

	public:
		virtual void Update(float fDeltaTime) override;
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;

	};
}
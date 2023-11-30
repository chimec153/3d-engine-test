#pragma once
#include "Bindable.h"

namespace Engine
{
	class ENGINE_DLL Texture :
		public Bindable
	{
	public:
		Texture();
		Texture(int iCount, int iSize, int iSlot, DXGI_FORMAT eFormat);
		Texture(const TCHAR* pFullPath, int iSlot = 0);
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
		std::unique_ptr<DirectX::ScratchImage> m_pImage;
		CPtr<ID3D11UnorderedAccessView> m_pUAV;

	public:
		int GetSlot()	const;
		bool LoadTexture(const TCHAR* pFileName, DirectX::ScratchImage& image);
		bool LoadTextureFromFullPath(const TCHAR* pFileName, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		bool LoadTextureFromFullPath(const std::vector<const TCHAR*>& pFileName);
		bool CreateTexture(const std::vector<DirectX::ScratchImage*>& image, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		bool CreateTexture(const DirectX::ScratchImage& image, D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT);
		bool CreateTexture(int iWidth, int iHeight, DXGI_FORMAT eFormat, int iMipLevels, int iArraySize, const D3D11_SUBRESOURCE_DATA* pData = nullptr, 
			D3D11_CPU_ACCESS_FLAG eCpuFlag = static_cast<D3D11_CPU_ACCESS_FLAG>(0), D3D11_USAGE eUsage = D3D11_USAGE_DEFAULT, D3D11_BIND_FLAG eFlag = D3D11_BIND_SHADER_RESOURCE);
		bool CreateShaderResourceView(DXGI_FORMAT eFormat, int iMipLevels, int iArraySize, D3D_SRV_DIMENSION eDimension = D3D11_SRV_DIMENSION_TEXTURE2D);
		bool CreateUnorderedAccessView(DXGI_FORMAT eFormat, D3D11_UAV_DIMENSION eDimension = D3D11_UAV_DIMENSION_TEXTURE2D);
		CPtr<ID3D11ShaderResourceView> GetSRV()	const;
		DirectX::ScratchImage* GetImage()	const;
		bool SaveTexture(const TCHAR* pFilePath, const std::string& strPathKey = TEXTURE_PATH);
		int GetImageWidth()	const noexcept;
		int GetImageHeight()	const noexcept;
		bool CreateTextureAndSRVAndUAV(int iWidth, int iHeight, DXGI_FORMAT eFormat, int iMipLevels = 1, int iArraySize = 1);
		void SetUAV(int iSlot);
		void ResetUAV(int iSlot);
		void ResetSRV();

	public:
		virtual void Update(float fDeltaTime) override;
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;
		virtual void PostBind() override;

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;

	};
}
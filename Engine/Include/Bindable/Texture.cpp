#include "Texture.h"
#include "../Core/PathManager.h"
#include "../Core/Window.h"

namespace Engine
{
	Texture::Texture() :
		Bindable()
		, m_pSRV(nullptr)
		, m_iSlot(0)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
	}

	Texture::Texture(int iWidth, int iHeight, int iSlot, DXGI_FORMAT eFormat)	:
		Bindable()
		, m_pSRV(nullptr)
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
		CreateTextureAndSRVAndUAV(iWidth, iHeight, eFormat);
	}

	Texture::Texture(const TCHAR* pFullPath, int iSlot) :
		Bindable()
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
		SetBindableType(BINDABLE_TYPE::TEXTURE);

		LoadTextureFromFullPath(pFullPath);
	}

	Texture::Texture(const char* pFileName, int iSlot) :
		Bindable()
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
		SetBindableType(BINDABLE_TYPE::TEXTURE);

		TCHAR strFullPath[MAX_PATH] = {};

#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, 0, pFileName, -1, strFullPath, MAX_PATH);
#else
		strcpy_s(strFullPath, pFullPath);
#endif

		LoadTextureFromFullPath(strFullPath);
	}

	Texture::Texture(const TCHAR* pFileName, const std::string& strPathKey, int iSlot, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage) :
		Bindable()
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
		SetBindableType(BINDABLE_TYPE::TEXTURE);

		TCHAR strFullPath[MAX_PATH] = {};

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		if (pPath)
		{
			wcscpy_s(strFullPath, pPath);
		}

		wcscat_s(strFullPath, pFileName);

		LoadTextureFromFullPath(strFullPath, eCpuFlag, eUsage);
	}

	Texture::Texture(const char* pFileName, const std::string& strPathKey, int iSlot) :
		Bindable()
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{
		SetBindableType(BINDABLE_TYPE::TEXTURE);

		char pFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPathKey);

		if (pPath)
		{
			strcpy_s(pFullPath, pPath);
		}

		strcat_s(pFullPath, pFileName);

		TCHAR strFullPath[MAX_PATH] = {};

#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, 0, pFullPath, -1, strFullPath, MAX_PATH);
#else
		strcpy_s(strFullPath, pFullPath);
#endif

		LoadTextureFromFullPath(strFullPath);
	}

	Texture::Texture(const std::vector<const TCHAR*>& pFileName, const std::string& strPathKey, int iSlot)
	{
		SetBindableType(BINDABLE_TYPE::TEXTURE);

		m_iSlot = iSlot;

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		std::vector<const TCHAR*> vecFileName;

		for (int i = 0; i < pFileName.size(); ++i)
		{
			TCHAR* strFullPath = dbg_new TCHAR[MAX_PATH];

			if (pPath)
			{
				_tcscpy_s(strFullPath, MAX_PATH, pPath);
			}

			_tcscat_s(strFullPath, MAX_PATH, pFileName[i]);

			vecFileName.push_back(strFullPath);
		}

		if (!LoadTextureFromFullPath(vecFileName))
		{
			Safe_Delete_VecList_Array(vecFileName);
			assert(false);
		}

		Safe_Delete_VecList_Array(vecFileName);
	}

	Texture::~Texture()
	{
	}

	int Texture::GetSlot() const
	{
		return m_iSlot;
	}

	bool Texture::LoadTexture(const TCHAR* strFullPath, DirectX::ScratchImage& image)
	{
		TCHAR strExt[_MAX_EXT] = {};

		_wsplitpath_s(strFullPath, nullptr, 0, nullptr, 0, nullptr, 0, strExt, _MAX_EXT);

		_wcsupr_s(strExt);

		if (!wcscmp(strExt, TEXT(".DDS")))
		{
			if (FAILED(DirectX::LoadFromDDSFile(strFullPath, DirectX::DDS_FLAGS_NONE, nullptr, image)))
			{
				//assert(false);
				return false;
			}
		}

		else if (!wcscmp(strExt, TEXT(".TGA")))
		{
			if (FAILED(DirectX::LoadFromTGAFile(strFullPath, nullptr, image)))
			{
				//assert(false);
				return false;
			}
		}

		else
		{
			if (FAILED(DirectX::LoadFromWICFile(strFullPath, DirectX::WIC_FLAGS_NONE, nullptr, image)))
			{
				assert(false);
				return false;
			}
		}
		return true;
	}

	bool Texture::LoadTextureFromFullPath(const TCHAR* strFullPath, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage)
	{
		_tcscpy_s(m_strFullPath, strFullPath);

		m_pImage = std::make_unique<DirectX::ScratchImage>();

		if (!LoadTexture(strFullPath, *m_pImage))
		{
			return false;
		}

		if (!CreateTexture(*m_pImage))
		{
			return false;
		}

		if (!CreateShaderResourceView(m_pImage->GetMetadata().format, static_cast<int>(m_pImage->GetMetadata().mipLevels), static_cast<int>(m_pImage->GetMetadata().arraySize), D3D11_SRV_DIMENSION_TEXTURE2D))
		{
			return false;
		}

		return true;
	}

	bool Texture::LoadTextureFromFullPath(const std::vector<const TCHAR*>& vecFileName)
	{
		std::vector<DirectX::Image> vecImage;
		std::vector<DirectX::ScratchImage*> vecSratchImage;

		for (int i = 0; i < static_cast<int>(vecFileName.size()); ++i)
		{
			DirectX::ScratchImage* pSratchImage = dbg_new DirectX::ScratchImage;

			if (!LoadTexture(vecFileName[i], *pSratchImage))
			{
				return false;
			}

			vecSratchImage.push_back(pSratchImage);
		}

		if (!CreateTexture(vecSratchImage))
		{
			Safe_Delete_VecList(vecSratchImage);
			return false;
		}

		if (!CreateShaderResourceView(vecSratchImage[0]->GetMetadata().format, static_cast<int>(vecSratchImage[0]->GetMetadata().mipLevels), static_cast<int>(vecSratchImage.size()), D3D11_SRV_DIMENSION_TEXTURE2DARRAY))
		{
			Safe_Delete_VecList(vecSratchImage);
			return false;
		}

		Safe_Delete_VecList(vecSratchImage);

		return true;
	}

	bool Texture::CreateTexture(const std::vector<DirectX::ScratchImage*>& image, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage)
	{
		std::vector<D3D11_SUBRESOURCE_DATA> vecSub(image[0]->GetMetadata().mipLevels * image.size());

		for (int i = 0; i < image.size(); ++i)
		{
			for (int j = 0; j < image[i]->GetMetadata().mipLevels; ++j)
			{
				const DirectX::Image* pImage = image[i]->GetImage(j, 0, 0);

				vecSub[i * image[0]->GetMetadata().mipLevels + j].pSysMem = pImage->pixels;
				vecSub[i * image[0]->GetMetadata().mipLevels + j].SysMemPitch = static_cast<unsigned int>(pImage->rowPitch);
			}
		}

		return CreateTexture(static_cast<int>(image[0]->GetMetadata().width), static_cast<int>(image[0]->GetMetadata().height), image[0]->GetMetadata().format, static_cast<int>(image[0]->GetMetadata().mipLevels), static_cast<int>(image.size()), &vecSub[0]);
	}

	bool Texture::CreateTexture(const DirectX::ScratchImage& image, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage)
	{
		std::vector<D3D11_SUBRESOURCE_DATA> vecSub(image.GetMetadata().arraySize * image.GetMetadata().mipLevels);

		for (int i = 0; i < image.GetMetadata().arraySize; ++i)
		{
			for (int j = 0; j < image.GetMetadata().mipLevels; ++j)
			{
				const DirectX::Image* pImage = image.GetImage(j, i, 0);

				vecSub[i * image.GetMetadata().mipLevels + j].pSysMem = pImage->pixels;
				vecSub[i * image.GetMetadata().mipLevels + j].SysMemPitch = static_cast<unsigned int>(pImage->rowPitch);
			}
		}

		return CreateTexture(static_cast<int>(image.GetMetadata().width), static_cast<int>(image.GetMetadata().height), image.GetMetadata().format, static_cast<int>(image.GetMetadata().mipLevels), static_cast<int>(image.GetMetadata().arraySize), &vecSub[0]);
	}

	bool Texture::CreateTexture(int iWidth, int iHeight, DXGI_FORMAT eFormat, int iMipLevels, int iArraySize, const D3D11_SUBRESOURCE_DATA* pData, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage, D3D11_BIND_FLAG eFlag)
	{
		D3D11_TEXTURE2D_DESC tTextureDesc = {};

		tTextureDesc.Format = eFormat;
		tTextureDesc.ArraySize = static_cast<UINT>(iArraySize);
		tTextureDesc.MipLevels = static_cast<UINT>(iMipLevels);
		tTextureDesc.Width = static_cast<UINT>(iWidth);
		tTextureDesc.Height = static_cast<UINT>(iHeight);
		tTextureDesc.BindFlags = eFlag;
		tTextureDesc.Usage = eUsage;
		tTextureDesc.SampleDesc.Count = 1;
		tTextureDesc.SampleDesc.Quality = 0;
		tTextureDesc.CPUAccessFlags = eCpuFlag;

		switch (tTextureDesc.Format)
		{
		case DXGI_FORMAT_BC1_UNORM:
			tTextureDesc.Width = tTextureDesc.Width + (4 - tTextureDesc.Width % 4) % 4;
			tTextureDesc.Height = tTextureDesc.Height + (4 - tTextureDesc.Height % 4) % 4;
			break;
		default:
			break;
		}

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateTexture2D(&tTextureDesc, pData, &m_pTexture)))
		{
			assert(false);
			return false;
		}

		return true;
	}

	bool Texture::CreateShaderResourceView(DXGI_FORMAT eFormat, int iMipLevels, int iArraySize, D3D_SRV_DIMENSION eDimension)
	{
		assert(m_pTexture);

		D3D11_SHADER_RESOURCE_VIEW_DESC tViewDesc = {};

		tViewDesc.Format = eFormat;

		switch (eDimension)
		{
		case D3D_SRV_DIMENSION_TEXTURE2D:
			tViewDesc.Texture2D.MipLevels = static_cast<unsigned int>(iMipLevels);
			break;
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			tViewDesc.Texture2DArray.MipLevels = static_cast<unsigned int>(iMipLevels);
			tViewDesc.Texture2DArray.ArraySize = static_cast<unsigned int>(iArraySize);
			break;
		}
		tViewDesc.ViewDimension = eDimension;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &tViewDesc, &m_pSRV)))
		{
			assert(false);
			return false;
		}

		return true;
	}

	bool Texture::CreateUnorderedAccessView(DXGI_FORMAT eFormat, D3D11_UAV_DIMENSION eDimension)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC tUAVDesc = {};

		tUAVDesc.Format = eFormat;
		tUAVDesc.ViewDimension = eDimension;

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), &tUAVDesc, &m_pUAV)))
		{
			assert(false);
			return false;
		}

		return true;
	}

	CPtr<ID3D11ShaderResourceView> Texture::GetSRV() const
	{
		return m_pSRV;
	}

	DirectX::ScratchImage* Texture::GetImage() const
	{
		return m_pImage.get();
	}

	bool Texture::SaveTexture(const TCHAR* pFilePath, const std::string& strPathKey)
	{
		TCHAR strFullPath[MAX_PATH] = {};

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		if (pPath) {
			_tcscpy_s(strFullPath, pPath);
		}

		_tcscat_s(strFullPath, pFilePath);

		if (FAILED(DirectX::SaveToWICFile(*m_pImage->GetImage(0, 0, 0), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), strFullPath, nullptr, nullptr))) {
			return false;
		}

		return true;
	}

	int Texture::GetImageWidth() const noexcept
	{
		if (!m_pImage)
		{
			return 0;
		}

		return static_cast<int>(m_pImage->GetMetadata().width);
	}

	int Texture::GetImageHeight() const noexcept
	{
		if (!m_pImage)
		{
			return 0;
		}

		return static_cast<int>(m_pImage->GetMetadata().height);
	}

	bool Texture::CreateTextureAndSRVAndUAV(int iWidth, int iHeight, DXGI_FORMAT eFormat, int iMipLevels, int iArraySize)
	{
		if (!CreateTexture(iWidth, iHeight, eFormat, iMipLevels, iArraySize, nullptr, (D3D11_CPU_ACCESS_FLAG)0, D3D11_USAGE_DEFAULT, (D3D11_BIND_FLAG)(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS)))
		{
			assert(false);
			return false;
		}

		if (!CreateShaderResourceView(eFormat, iMipLevels, iArraySize))
		{
			assert(false);
			return false;
		}

		if (!CreateUnorderedAccessView(eFormat))
		{
			assert(false);
			return false;
		}

		return true;
	}

	void Texture::SetUAV(int iSlot)
	{
		Graphics::GetInst()->GetDeviceContext()->CSSetUnorderedAccessViews(iSlot, 1, m_pUAV.GetAddressof(), nullptr);
	}

	void Texture::ResetUAV(int iSlot)
	{
		ID3D11UnorderedAccessView* pUAV = nullptr;

		Graphics::GetInst()->GetDeviceContext()->CSSetUnorderedAccessViews(iSlot, 1, &pUAV, nullptr);
	}

	void Texture::ResetSRV()
	{
		ID3D11ShaderResourceView* pSRV = nullptr;

		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, 1, &pSRV);
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, 1, &pSRV);
	}

	void Texture::Update(float fDeltaTime)
	{
	}

	void Texture::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
	}

	std::shared_ptr<Bindable> Texture::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
	}

	void Texture::PostBind()
	{
	}

	void Texture::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_iSlot, 4, 1, pFile);

		short iLength = static_cast<short>(_tcslen(m_strFullPath));

		fwrite(&iLength, 2, 1, pFile);

		if (iLength)
		{
			fwrite(m_strFullPath, sizeof(TCHAR), iLength, pFile);
		}
	}

	void Texture::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_iSlot, 4, 1, pFile);

		short iLength;

		fread(&iLength, 2, 1, pFile);

		if (iLength)
		{
			fread(m_strFullPath, sizeof(TCHAR), iLength, pFile);
		}

		LoadTextureFromFullPath(m_strFullPath);
	}
}
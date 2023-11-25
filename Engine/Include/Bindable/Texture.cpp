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

	Texture::Texture(int iCount, int iSize, int iSlot, void* pData)	:
		Bindable()
		, m_pSRV(nullptr)
		, m_iSlot(iSlot)
		, m_strFullPath()
		, m_pTexture(nullptr)
		, m_pImage(nullptr)
	{

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
				return false;
			}
		}

		else if (!wcscmp(strExt, TEXT(".TGA")))
		{
			if (FAILED(DirectX::LoadFromTGAFile(strFullPath, nullptr, image)))
			{
				return false;
			}
		}

		else
		{
			if (FAILED(DirectX::LoadFromWICFile(strFullPath, DirectX::WIC_FLAGS_NONE, nullptr, image)))
			{
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

		if (!CreateShaderResourceView(*m_pImage, D3D11_SRV_DIMENSION_TEXTURE2D, eCpuFlag, eUsage))
		{
			return false;
		}

		return true;
	}

	bool Texture::LoadTextureFromFullPath(const std::vector<const TCHAR*>& vecFileName)
	{
		std::vector<DirectX::Image> vecImage;
		std::vector<DirectX::ScratchImage> vecSratchImage;

		for (int i = 0; i < static_cast<int>(vecFileName.size()); ++i)
		{
			vecSratchImage.emplace_back();

			if (!LoadTexture(vecFileName[i], vecSratchImage.back()))
			{
				return false;
			}

			vecImage.push_back(*vecSratchImage.back().GetImage(0, 0, 0));
		}
		DirectX::ScratchImage tTotalImage;

		HRESULT hr = tTotalImage.InitializeArrayFromImages(&vecImage[0], vecImage.size());

		if (FAILED(hr))
		{
			assert(false);
			return false;
		}

		if (!CreateShaderResourceView(tTotalImage, D3D11_SRV_DIMENSION_TEXTURE2DARRAY))
		{
			return false;
		}

		return true;
	}

	bool Texture::CreateShaderResourceView(const DirectX::ScratchImage& image, D3D_SRV_DIMENSION eDimension, D3D11_CPU_ACCESS_FLAG eCpuFlag, D3D11_USAGE eUsage)
	{
		D3D11_TEXTURE2D_DESC tTextureDesc = {};

		tTextureDesc.Format = image.GetMetadata().format;
		tTextureDesc.ArraySize = static_cast<UINT>(image.GetMetadata().arraySize);
		tTextureDesc.MipLevels = static_cast<UINT>(image.GetMetadata().mipLevels);
		tTextureDesc.Width = static_cast<UINT>(image.GetMetadata().width);
		tTextureDesc.Height = static_cast<UINT>(image.GetMetadata().height);
		tTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
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

		if (FAILED(Graphics::GetInst()->GetDevice()->CreateTexture2D(&tTextureDesc, &vecSub[0], &m_pTexture)))
		{
			assert(false);
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC tViewDesc = {};

		tViewDesc.Format = tTextureDesc.Format;

		switch (eDimension)
		{
		case D3D_SRV_DIMENSION_TEXTURE2D:
			tViewDesc.Texture2D.MipLevels = static_cast<unsigned int>(image.GetMetadata().mipLevels);
			break;
		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			tViewDesc.Texture2DArray.MipLevels = static_cast<unsigned int>(image.GetMetadata().mipLevels);
			tViewDesc.Texture2DArray.ArraySize = static_cast<unsigned int>(image.GetMetadata().arraySize);
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

	void Texture::Update(float fDeltaTime)
	{
	}

	void Texture::Bind()
	{
		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, 1, m_pSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, 1, m_pSRV.GetAdressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, 1, m_pSRV.GetAdressof());
	}

	std::shared_ptr<Bindable> Texture::Clone()
	{
		return std::static_pointer_cast<Bindable>(shared_from_this());
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
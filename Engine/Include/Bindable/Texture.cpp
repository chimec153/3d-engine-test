#include "Texture.h"
#include "../Core/PathManager.h"
#include "../Core/Window.h"
#include "BindableManager.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdint>

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

		CPathManager::GetInst()->Resolve(pFileName, strPathKey, strFullPath);

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

		CPathManager::GetInst()->ResolveMB(pFileName, strPathKey, pFullPath);

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

		std::vector<const TCHAR*> vecFileName;

		for (int i = 0; i < pFileName.size(); ++i)
		{
			TCHAR* strFullPath = dbg_new TCHAR[MAX_PATH];

			CPathManager::GetInst()->Resolve(pFileName[i], strPathKey, strFullPath);

			vecFileName.push_back(strFullPath);
		}

		if (!LoadTextureFromFullPath(vecFileName))
		{
			assert(false);
			Safe_Delete_VecList_Array(vecFileName);
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
				//assert(false);
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
			char buf[1024];
			char narrow[MAX_PATH] = {};
			WideCharToMultiByte(CP_ACP, 0, strFullPath, -1, narrow, MAX_PATH, nullptr, nullptr);
			sprintf_s(buf, "[Texture] FAILED to load: '%s'\n", narrow);
			::OutputDebugStringA(buf);
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

		CPathManager::GetInst()->Resolve(pFilePath, strPathKey, strFullPath);

		if (FAILED(DirectX::SaveToWICFile(*m_pImage->GetImage(0, 0, 0), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), strFullPath, nullptr, nullptr))) {
			return false;
		}

		return true;
	}

	bool Texture::SaveTexture2D(const TCHAR* pFilePath, const std::string& strPathKey)
	{
		if (!m_pTexture)
		{
			return false;
		}

		TCHAR strFullPath[MAX_PATH] = {};

		CPathManager::GetInst()->Resolve(pFilePath, strPathKey, strFullPath);

		DirectX::ScratchImage image;

		if (FAILED(DirectX::CaptureTexture(Graphics::GetInst()->GetDevice(), Graphics::GetInst()->GetDeviceContext(), m_pTexture.Get(), image)))
		{
			return false;
		}

		if (FAILED(DirectX::SaveToWICFile(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_BMP), strFullPath, nullptr, nullptr)))
		{
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
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		if (m_iSlot >= 0 && m_iSlot < BindCache::kTextureSlots) cache.pBoundTextures[m_iSlot] = nullptr;
	}

	const TCHAR* Texture::GetFullPath() const
	{
		return m_strFullPath;
	}

	void Texture::Update(float fDeltaTime)
	{
	}

	void Texture::Bind()
	{
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		ID3D11ShaderResourceView* mine = *m_pSRV;
		if (m_iSlot >= 0 && m_iSlot < BindCache::kTextureSlots && cache.pBoundTextures[m_iSlot] == mine)
			return;
		Graphics::GetInst()->GetDeviceContext()->VSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->PSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
		Graphics::GetInst()->GetDeviceContext()->CSSetShaderResources(m_iSlot, 1, m_pSRV.GetAddressof());
		if (m_iSlot >= 0 && m_iSlot < BindCache::kTextureSlots) cache.pBoundTextures[m_iSlot] = mine;
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

		//LoadTextureFromFullPath(m_strFullPath);
	}

	// Named (not anonymous) detail namespace: Engine is a unity/jumbo build, so
	// file-local helpers must not collide across the merged translation unit.
	namespace texture_proc_detail
	{
		inline uint32_t Hash(uint32_t x)
		{
			x ^= x >> 16; x *= 0x7feb352dU;
			x ^= x >> 15; x *= 0x846ca68bU;
			x ^= x >> 16;
			return x;
		}
		inline float Rand01(int ix, int iy, int seed)
		{
			uint32_t h = Hash((uint32_t)ix * 374761393u + (uint32_t)iy * 668265263u + (uint32_t)seed * 362437u);
			return (h & 0xFFFFFFu) / (float)0xFFFFFFu;
		}
		inline float Smooth(float t) { return t * t * (3.f - 2.f * t); }
		inline float ValueNoise(float x, float y, int seed)
		{
			int x0 = (int)floorf(x), y0 = (int)floorf(y);
			float fx = Smooth(x - (float)x0), fy = Smooth(y - (float)y0);
			float v00 = Rand01(x0, y0, seed),     v10 = Rand01(x0 + 1, y0, seed);
			float v01 = Rand01(x0, y0 + 1, seed), v11 = Rand01(x0 + 1, y0 + 1, seed);
			float a = v00 + (v10 - v00) * fx;
			float b = v01 + (v11 - v01) * fx;
			return a + (b - a) * fy;
		}
		inline float Fbm(float x, float y, int seed)
		{
			float sum = 0.f, amp = 0.5f, freq = 1.f;
			for (int o = 0; o < 4; ++o)
			{
				sum += amp * ValueNoise(x * freq, y * freq, seed + o * 101);
				freq *= 2.f; amp *= 0.5f;
			}
			return sum; // ~[0,1)
		}
		inline uint32_t PackRGBA(float r, float g, float b, float a)
		{
			auto c = [](float v) -> uint32_t { int i = (int)(v * 255.f + 0.5f); return (uint32_t)(i < 0 ? 0 : (i > 255 ? 255 : i)); };
			return c(r) | (c(g) << 8) | (c(b) << 16) | (c(a) << 24); // R8G8B8A8_UNORM little-endian
		}
	}

	bool GenerateProceduralTexturePNG(ProcPattern pattern, const ProcTexParams& p,
		const TCHAR* pRelPath, const std::string& strPathKey,
		TCHAR* pOutFullPath, int iOutCap)
	{
		using namespace texture_proc_detail;

		const int w = p.width  > 0 ? p.width  : 1;
		const int h = p.height > 0 ? p.height : 1;
		const float A[4] = { p.colorA[0], p.colorA[1], p.colorA[2], p.colorA[3] };
		const float B[4] = { p.colorB[0], p.colorB[1], p.colorB[2], p.colorB[3] };
		const float scale = p.scale > 0.0001f ? p.scale : 1.f;

		std::vector<uint32_t> pixels((size_t)w * (size_t)h);

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const float u = (x + 0.5f) / (float)w;
				const float v = (y + 0.5f) / (float)h;
				float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

				switch (pattern)
				{
				case ProcPattern::SolidValue:
					r = A[0]; g = A[1]; b = A[2]; a = A[3];
					break;
				case ProcPattern::Checker:
				{
					int cx = (int)floorf(u * scale), cy = (int)floorf(v * scale);
					const float* c = ((cx + cy) & 1) ? B : A;
					r = c[0]; g = c[1]; b = c[2]; a = c[3];
					break;
				}
				case ProcPattern::Gradient:
				{
					float t = v;
					r = A[0] + (B[0] - A[0]) * t; g = A[1] + (B[1] - A[1]) * t;
					b = A[2] + (B[2] - A[2]) * t; a = A[3] + (B[3] - A[3]) * t;
					break;
				}
				case ProcPattern::ValueNoise:
				{
					float n = Fbm(u * scale, v * scale, p.seed);
					n = (n - 0.5f) * p.strength + 0.5f;
					n = n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
					r = A[0] + (B[0] - A[0]) * n; g = A[1] + (B[1] - A[1]) * n;
					b = A[2] + (B[2] - A[2]) * n; a = 1.f;
					break;
				}
				case ProcPattern::Brick:
				{
					float rowF = v * scale * 0.5f;          // rows = half the brick count
					int   row  = (int)floorf(rowF);
					float offset = (row & 1) ? 0.5f : 0.f;  // running bond
					float colF = u * scale + offset;
					float fxc = colF - floorf(colF);
					float fyc = rowF - floorf(rowF);
					const float mortar = 0.08f;             // line thickness (cell fraction)
					bool isMortar = (fxc < mortar || fxc > 1.f - mortar || fyc < mortar || fyc > 1.f - mortar);
					const float* c = isMortar ? B : A;
					r = c[0]; g = c[1]; b = c[2]; a = c[3];
					break;
				}
				case ProcPattern::NormalFromNoise:
				{
					float hC = Fbm(u * scale, v * scale, p.seed);
					float hX = Fbm((u + 1.f / w) * scale, v * scale, p.seed);
					float hY = Fbm(u * scale, (v + 1.f / h) * scale, p.seed);
					float dx = (hX - hC) * p.strength * 8.f;
					float dy = (hY - hC) * p.strength * 8.f;
					float nx = -dx, ny = -dy, nz = 1.f;
					float inv = 1.f / sqrtf(nx * nx + ny * ny + nz * nz);
					nx *= inv; ny *= inv; nz *= inv;
					r = nx * 0.5f + 0.5f; g = ny * 0.5f + 0.5f; b = nz * 0.5f + 0.5f; a = 1.f;
					break;
				}
				default:
					break;
				}

				pixels[(size_t)y * w + x] = PackRGBA(r, g, b, a);
			}
		}

		DirectX::ScratchImage img;
		if (FAILED(img.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, w, h, 1, 1)))
		{
			return false;
		}
		const DirectX::Image* dst = img.GetImage(0, 0, 0);
		for (int y = 0; y < h; ++y)
		{
			memcpy(dst->pixels + (size_t)y * dst->rowPitch, &pixels[(size_t)y * w], (size_t)w * 4);
		}

		TCHAR strFullPath[MAX_PATH] = {};
		CPathManager::GetInst()->Resolve(pRelPath, strPathKey, strFullPath);

		if (FAILED(DirectX::SaveToWICFile(*dst, DirectX::WIC_FLAGS_NONE,
			DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), strFullPath, nullptr, nullptr)))
		{
			return false;
		}

		if (pOutFullPath && iOutCap > 0)
		{
			_tcsncpy_s(pOutFullPath, iOutCap, strFullPath, _TRUNCATE);
		}
		return true;
	}
}
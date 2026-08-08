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
		// Raw ID3D11Texture2D access for D2D/DXGI interop. Used by
		// Text (and any future ad-hoc renderer) to query an
		// IDXGISurface and stand up an ID2D1RenderTarget on top of
		// this texture, so D2D draws land in the same memory the
		// UI pixel shader samples — no extra copy.
		CPtr<ID3D11Texture2D> GetTexture2D() const { return m_pTexture; }
		DirectX::ScratchImage* GetImage()	const;
		bool SaveTexture(const TCHAR* pFilePath, const std::string& strPathKey = TEXTURE_PATH);
		bool SaveTexture2D(const TCHAR* pFilePath, const std::string& strPathKey = TEXTURE_PATH);
		int GetImageWidth()	const noexcept;
		int GetImageHeight()	const noexcept;
		bool CreateTextureAndSRVAndUAV(int iWidth, int iHeight, DXGI_FORMAT eFormat, int iMipLevels = 1, int iArraySize = 1);
		void SetUAV(int iSlot);
		void ResetUAV(int iSlot);
		void ResetSRV();
		const TCHAR* GetFullPath()	const;

	public:
		virtual void Update(float fDeltaTime) override;
		virtual void Bind() override;
		virtual std::shared_ptr<Bindable> Clone() override;
		virtual void PostBind() override;
		// Phase E7 — sort-by-state cache moved to Graphics::BindCache. Tracks
		// the SRV currently set at each slot. Texture binding sets VS+PS+CS
		// at the same slot to the same SRV, so a single tracker per slot
		// is sufficient.

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;

	};

	// === Procedural material-texture authoring ===
	// CPU-generates a pattern into an RGBA8 buffer and saves it as a PNG under
	// strPathKey (default Resource/Texture). The caller then loads it back as a
	// normal slotted Texture (same path the editor's "Set" button uses), so it
	// persists through the existing .mat pipeline with zero serialization change.
	// For single-channel maps (Roughness/Metalness/AO) the value is written into
	// all of R/G/B; the PBR shader samples .r, so it reads correctly.
	enum class ProcPattern
	{
		SolidValue = 0,   // flat colorA
		Checker,          // colorA/colorB squares (scale = cells across)
		ValueNoise,       // fBm noise, lerp colorA..colorB (strength = contrast)
		Gradient,         // vertical colorA(top)..colorB(bottom)
		Brick,            // brick field, colorA brick / colorB mortar
		NormalFromNoise,  // tangent-space normal map baked from noise height
	};

	struct ProcTexParams
	{
		int   width    = 256;
		int   height   = 256;
		float colorA[4] = { 1.f, 1.f, 1.f, 1.f };
		float colorB[4] = { 0.f, 0.f, 0.f, 1.f };
		float scale    = 8.f;    // cells (checker/brick) or frequency (noise)
		int   seed     = 1337;
		float strength = 1.f;    // noise contrast / normal-map strength
	};

	// Returns true on success. When pOutFullPath/iOutCap are supplied, the
	// resolved absolute file path is copied out so the caller can load it.
	ENGINE_DLL bool GenerateProceduralTexturePNG(ProcPattern pattern, const ProcTexParams& params,
		const TCHAR* pRelPath, const std::string& strPathKey,
		TCHAR* pOutFullPath = nullptr, int iOutCap = 0);
}
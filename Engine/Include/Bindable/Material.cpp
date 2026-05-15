#include "Material.h"
#include "BindableManager.h"
#include "ConstantBuffer.h"
#include "Texture.h"
#include "../Core/Graphics.h"

namespace Engine
{
	// Slot index N → t-register in shared.hlsl. Keep this in sync with
	// the comment block on Material::kMaterialSlotCount in the header.
	const int Material::kMaterialSlotRegisters[Material::kMaterialSlotCount] = {
		0,  // Diffuse
		1,  // Normal
		2,  // Specular
		3,  // Emissive
		6,  // Roughness
		8,  // AO
		9,  // Metalness
	};

	int Material::SlotRegisterToIndex(int iRegister)
	{
		for (int i = 0; i < kMaterialSlotCount; ++i)
		{
			if (kMaterialSlotRegisters[i] == iRegister) return i;
		}
		return -1;
	}

	Material::Material() :
		Bindable()
		, m_tMaterial()
		, m_pConstantBuffer(StaticFindBindable<ConstantBuffer<MATERIAL>>("Material"))
	{
		SetBindableType(BINDABLE_TYPE::MATERIAL);

		m_tMaterial.diffuseColor.x = 1.f;
		m_tMaterial.diffuseColor.y = 1.f;
		m_tMaterial.diffuseColor.z = 1.f;
		m_tMaterial.diffuseColor.w = 1.f;

		m_tMaterial.ambientColor.x = 1.f;
		m_tMaterial.ambientColor.y = 1.f;
		m_tMaterial.ambientColor.z = 1.f;
		m_tMaterial.ambientColor.w = 1.f;

		// PBR dielectric F0 ≈ 0.04 (plastic, wood, fabric, skin, ...).
		// Used in PS_Multi as GetFresnel(LDotH, vSpecColor) — vSpecColor is
		// the F0 in the specular workflow. Metals override this via the
		// metalness-texture branch in anisotropic_microfacet.hlsl PS.
		m_tMaterial.specularColor.x = 0.04f;
		m_tMaterial.specularColor.y = 0.04f;
		m_tMaterial.specularColor.z = 0.04f;
		m_tMaterial.specularColor.w = 1.f;

		m_tMaterial.emissiveColor.x = 1.f;
		m_tMaterial.emissiveColor.y = 1.f;
		m_tMaterial.emissiveColor.z = 1.f;
		m_tMaterial.emissiveColor.w = 1.f;

		m_tMaterial.fSpecPower = 100;
		m_tMaterial.fFraction = 0.5f;
		m_tMaterial.vRoughness.x = 0.5f;
		m_tMaterial.vRoughness.y = 0.1f;
	}

	Material::Material(const std::shared_ptr<ConstantBuffer<MATERIAL>>& pBuffer) :
		Bindable()
		, m_tMaterial()
		, m_pConstantBuffer(pBuffer)
	{
		SetBindableType(BINDABLE_TYPE::MATERIAL);

		m_tMaterial.diffuseColor.w = 1.f;
		// PBR dielectric F0 default — matches the value above.
		m_tMaterial.specularColor.x = 0.04f;
		m_tMaterial.specularColor.y = 0.04f;
		m_tMaterial.specularColor.z = 0.04f;
		m_tMaterial.specularColor.w = 1.f;
	}

	Material::Material(const Material& material) :
		Bindable(material)
		, m_tMaterial(material.m_tMaterial)
		, m_pConstantBuffer(material.m_pConstantBuffer)
		, m_vecTexture(material.m_vecTexture)
	{
	}

	void Material::SetDiffuseColor(float r, float g, float b, float w)
	{
		m_tMaterial.diffuseColor.x = r;
		m_tMaterial.diffuseColor.y = g;
		m_tMaterial.diffuseColor.z = b;
		m_tMaterial.diffuseColor.w = w;
	}

	void Material::SetAmbientColor(float r, float g, float b, float w)
	{
		m_tMaterial.ambientColor.x = r;
		m_tMaterial.ambientColor.y = g;
		m_tMaterial.ambientColor.z = b;
		m_tMaterial.ambientColor.w = w;
	}

	void Material::SetSpecularColor(float r, float g, float b, float w)
	{
		m_tMaterial.specularColor.x = r;
		m_tMaterial.specularColor.y = g;
		m_tMaterial.specularColor.z = b;
		m_tMaterial.specularColor.w = w;
	}

	void Material::SetDiffuseColor(const Vector4& color)
	{
		m_tMaterial.diffuseColor = color;
	}

	void Material::SetAmbientColor(const Vector4& color)
	{
		m_tMaterial.ambientColor = color;
	}

	void Material::SetSpecularColor(const Vector4& color)
	{
		m_tMaterial.specularColor = color;
	}

	void Material::SetEmissiveColor(const Vector4& color)
	{
		m_tMaterial.emissiveColor = color;
	}

	void Material::SetShininess(float fShininess)
	{
		m_tMaterial.fSpecPower = fShininess;
	}

	void Material::SetReflectivity(float fReflectivity)
	{
		m_tMaterial.fFraction = fReflectivity;
	}

	void Material::SetRandomColor()
	{
		m_tMaterial.diffuseColor.x = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.y = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.z = rand() % 256 / 255.f;
		m_tMaterial.diffuseColor.w = 1.f;

		m_tMaterial.ambientColor.x = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.y = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.z = rand() % 256 / 255.f;
		m_tMaterial.ambientColor.w = 1.f;

		m_tMaterial.specularColor.x = rand() % 256 / 255.f;
		m_tMaterial.specularColor.y = rand() % 256 / 255.f;
		m_tMaterial.specularColor.z = rand() % 256 / 255.f;
		m_tMaterial.specularColor.w = 1.f;

		m_tMaterial.fSpecPower = rand() % 200 + 1.f;
		m_tMaterial.fFraction = rand() % 201 / 200.f;
		m_tMaterial.vRoughness.x = rand() % 201 / 200.f;
		m_tMaterial.vRoughness.y = rand() % 201 / 200.f;
	}

	const MATERIAL& Material::GetMaterial() const
	{
		return m_tMaterial;
	}

	void Material::SetMaterial(const MATERIAL& mtrl)
	{
		m_tMaterial = mtrl;
	}

	void Material::SetRoughnessX(float x)
	{
		m_tMaterial.vRoughness.x = x;
	}

	void Material::SetRoughnessY(float y)
	{
		m_tMaterial.vRoughness.y = y;
	}

	void Material::UsePaperBurn()
	{
		m_tMaterial.bUsePaperBurn = true;
	}

	void Material::SetTexture(int iSlotIdx, const std::shared_ptr<Texture>& pTexture)
	{
		if (iSlotIdx < 0 || iSlotIdx >= kMaterialSlotCount) return;
		m_vecTexture[iSlotIdx] = pTexture;
	}

	std::shared_ptr<Texture> Material::GetTexture(int iSlotIdx) const
	{
		if (iSlotIdx < 0 || iSlotIdx >= kMaterialSlotCount) return nullptr;
		return m_vecTexture[iSlotIdx];
	}

	void Material::Update(float fDeltaTime)
	{
	}

	void Material::Bind()
	{
		m_pConstantBuffer->UpdateBuffer(m_tMaterial);

		m_pConstantBuffer->Bind();

		// Per-slot texture binding. Each slot has a fixed t-register; an
		// empty slot pushes a null SRV so HLSL `GetDimensions` reports
		// (0,0) and the shader takes the uniform-fallback branch. Empty
		// slots also wipe any stale SRV the previous mesh left behind.
		BindCache& cache = Graphics::GetInst()->GetBindCache();
		auto* ctx = Graphics::GetInst()->GetDeviceContext();
		for (int i = 0; i < kMaterialSlotCount; ++i)
		{
			int iReg = kMaterialSlotRegisters[i];
			if (m_vecTexture[i])
			{
				// Texture::Bind handles cache short-circuit + slot-from-tex.
				// Sanity: keep tex's m_iSlot in sync with its array index.
				m_vecTexture[i]->Bind();
			}
			else if (iReg >= 0 && iReg < BindCache::kTextureSlots &&
				cache.pBoundTextures[iReg])
			{
				ID3D11ShaderResourceView* pNull = nullptr;
				ctx->VSSetShaderResources(iReg, 1, &pNull);
				ctx->PSSetShaderResources(iReg, 1, &pNull);
				ctx->CSSetShaderResources(iReg, 1, &pNull);
				cache.pBoundTextures[iReg] = nullptr;
			}
		}
	}

	std::shared_ptr<Bindable> Material::Clone()
	{
		return std::make_shared<Material>(*this);
	}

	void Material::Save(FILE* pFile)
	{
		// Format: [tag from Bindable::Save][80 bytes MATERIAL][7 slots].
		// Per slot: 1 byte present-flag + Texture::Save() if present.
		__super::Save(pFile);

		fwrite(&m_tMaterial, 80, 1, pFile);

		for (int i = 0; i < kMaterialSlotCount; ++i)
		{
			uint8_t bHas = m_vecTexture[i] ? 1 : 0;
			fwrite(&bHas, 1, 1, pFile);
			if (bHas)
			{
				m_vecTexture[i]->Save(pFile);
			}
		}
	}

	void Material::Load(FILE* pFile)
	{
		// Reads the new-format payload (tag + 80 bytes + 7 slots). Older
		// .mesh files lack the slot block — Mesh::Load's old-format branch
		// must call LoadLegacy instead so we don't misread the next
		// container's bytes as our texture slots.
		__super::Load(pFile);

		fread(&m_tMaterial, 80, 1, pFile);

		for (int i = 0; i < kMaterialSlotCount; ++i)
		{
			uint8_t bHas = 0;
			fread(&bHas, 1, 1, pFile);
			if (!bHas) continue;

			auto pTex = std::make_shared<Texture>();
			pTex->Load(pFile);

			if (auto pPrev = BindableManager<Texture>::GetInst()->FindBindable(pTex->GetTag()))
			{
				pTex = pPrev;
			}
			else if (pTex->LoadTextureFromFullPath(pTex->GetFullPath()))
			{
				BindableManager<Texture>::GetInst()->AddBindable(pTex);
			}
			m_vecTexture[i] = pTex;
		}
	}

	void Material::LoadLegacy(FILE* pFile)
	{
		// Pre-refactor wire format: tag + 80-byte MATERIAL struct, nothing
		// after. Texture slots are filled in by Mesh::Load externally from
		// the container's loose texture list.
		__super::Load(pFile);

		fread(&m_tMaterial, 80, 1, pFile);
	}
}
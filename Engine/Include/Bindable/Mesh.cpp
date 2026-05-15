#include "Mesh.h"
#include "Animation.h"
#include "MeshUtils.h"
#include "../Core/Graphics.h"
#include "BindableManager.h"

namespace Engine
{

	Mesh::Mesh(int iCount)	:
		Bindable()
	{
		SetBindableType(Engine::BINDABLE_TYPE::MESH);

		m_vecMeshContainer.resize(iCount);
	}
	Mesh::Mesh(const Mesh& mesh)	:
		Bindable(mesh)
		, m_vecMeshContainer(mesh.m_vecMeshContainer)
		, m_vBoundingSphereInfo(mesh.m_vBoundingSphereInfo)
	{
		// Material is now a shared asset (BindableManager<Material> SSoT).
		// Don't clone — every Mesh instance points at the same Material
		// shared_ptr, and per-instance overrides live on MeshRendererComponent.
	}
	const Vector4& Mesh::GetBoundingSphereInfo() const
	{
		return m_vBoundingSphereInfo;
	}
	int Mesh::GetMeshCount() const
	{
		return static_cast<int>(m_vecMeshContainer.size());
	}
	int Mesh::GetMeshSubCount(int iIndex) const
	{
		return static_cast<int>(m_vecMeshContainer[iIndex].m_vecIndexBuffer.size());
	}
#ifdef _DEBUG
	bool Mesh::IsMeshEnabled(int iIndex) const
	{
		return m_vecMeshContainer[iIndex].bEnable;
	}
	void Mesh::ToggleMesh(int iIndex)
	{
		m_vecMeshContainer[iIndex].bEnable ^= true;
	}
	void Mesh::SetMeshSubOffset(int iIndex, int iSubIndex, int iOffset)
	{
		m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].iOffset = iOffset;
	}
	int Mesh::GetMeshSubOffset(int iIndex, int iSubIndex) const
	{
		return m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].iOffset;
	}
#endif

	void Mesh::SetVertexCount(int iIndex, int iCount)
	{
		m_vecMeshContainer[iIndex].m_iCount = iCount;
	}

	void Mesh::AddMaterial(int iIndex, const std::shared_ptr<Material>& pMaterial)
	{
		if (m_vecMeshContainer.size() <= iIndex || iIndex < 0)
		{
			assert(false);
			return;
		}

		m_vecMeshContainer[iIndex].vecMaterial.push_back(pMaterial);
	}

	std::shared_ptr<Material> Mesh::GetMaterial(int iIndex, int iSubIndex) const
	{
		if (m_vecMeshContainer.size() <= iIndex ||
			iIndex < 0 ||
			m_vecMeshContainer[iIndex].vecMaterial.size() <= iSubIndex ||
			iSubIndex < 0)
		{
			return nullptr;
		}

		return m_vecMeshContainer[iIndex].vecMaterial[iSubIndex];
	}

	void Mesh::SetMaterial(int iIndex, int iSubIndex, std::shared_ptr<Material> pMaterial)
	{
		if (m_vecMeshContainer.size() <= iIndex ||
			m_vecMeshContainer[iIndex].vecMaterial.size() <= iSubIndex ||
			iIndex < 0 || iSubIndex < 0)
		{
			assert(false);
			return;
		}

		m_vecMeshContainer[iIndex].vecMaterial[iSubIndex] = pMaterial;
	}

	bool Mesh::SetVertexBuffer(int iIndex, const void* pData, int iSize)
	{
		D3D11_MAPPED_SUBRESOURCE tSub = {};

		if (FAILED(Graphics::GetInst()->GetDeviceContext()->Map(m_vecMeshContainer[iIndex].m_pVertexBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &tSub))) {
			return false;
		}

		memcpy_s(tSub.pData, m_vecMeshContainer[iIndex].m_iSize * m_vecMeshContainer[iIndex].m_iCount, pData, iSize);

		Graphics::GetInst()->GetDeviceContext()->Unmap(m_vecMeshContainer[iIndex].m_pVertexBuffer.Get(), 0);

		return true;
	}

	bool Mesh::SetIndexBuffer(int iIndex, int iSubIndex, const void* pData, int iSize)
	{
		D3D11_MAPPED_SUBRESOURCE tSub = {};

		if (FAILED(Graphics::GetInst()->GetDeviceContext()->Map(m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].pBuffer.Get(), 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &tSub))) {
			return false;
		}

		int iDataSize = 0;

		switch (m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].eFormat)
		{
		case DXGI_FORMAT_R32_UINT:
			iDataSize = 4;
			break;
		case DXGI_FORMAT_R16_UINT:
			iDataSize = 2;
			break;
		}

		memcpy_s(tSub.pData, m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].iCount * iDataSize, pData, iSize);

		Graphics::GetInst()->GetDeviceContext()->Unmap(m_vecMeshContainer[iIndex].m_vecIndexBuffer[iSubIndex].pBuffer.Get(), 0);

		return true;
	}

	void Mesh::UsePaperBurn()
	{
		for (int i = 0; i < static_cast<int>(m_vecMeshContainer.size()); ++i)
		{
			for (int j = 0; j < static_cast<int>(m_vecMeshContainer[i].vecMaterial.size()); ++j)
			{
				m_vecMeshContainer[i].vecMaterial[j]->UsePaperBurn();
			}
		}
	}

	void Mesh::Bind()
	{
	}

	void Mesh::Draw(const MaterialResolver& resolver)
	{
		for (int i = 0; i < static_cast<int>(m_vecMeshContainer.size()); ++i)
		{
#ifdef _DEBUG
			if (!m_vecMeshContainer[i].bEnable)
			{
				continue;
			}
#endif

			// Per-container texture binding is gone — each Material binds
			// its own textures (and wipes stale SRVs for empty slots) in
			// Material::Bind below.

			UINT iStride = m_vecMeshContainer[i].m_iSize;
			UINT iOffset = 0;

			Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 1, m_vecMeshContainer[i].m_pVertexBuffer.GetAddressof(), &iStride, &iOffset);

			for (int j = 0; j < m_vecMeshContainer[i].m_vecIndexBuffer.size(); ++j)
			{
				// Resolver gets first pick (per-slot override on the renderer
				// component); mesh's own material is the fallback.
				std::shared_ptr<Material> pMat;
				if (resolver) pMat = resolver(i, j);
				if (!pMat && m_vecMeshContainer[i].vecMaterial.size() > j)
					pMat = m_vecMeshContainer[i].vecMaterial[j];
				if (pMat) pMat->Bind();

				Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(*m_vecMeshContainer[i].m_vecIndexBuffer[j].pBuffer, m_vecMeshContainer[i].m_vecIndexBuffer[j].eFormat, 0);

				if (m_vecMeshContainer[i].m_vecIndexBuffer[j].pBuffer)
				{
#ifdef _DEBUG
					Graphics::GetInst()->GetDeviceContext()->DrawIndexed(m_vecMeshContainer[i].m_vecIndexBuffer[j].iCount - m_vecMeshContainer[i].m_vecIndexBuffer[j].iOffset, m_vecMeshContainer[i].m_vecIndexBuffer[j].iOffset, 0);
#else
					Graphics::GetInst()->GetDeviceContext()->DrawIndexed(m_vecMeshContainer[i].m_vecIndexBuffer[j].iCount, 0, 0);
#endif
				}
				else
				{
					Graphics::GetInst()->GetDeviceContext()->Draw(m_vecMeshContainer[i].m_iCount, 0);
				}
			}

			if (m_vecMeshContainer[i].m_vecIndexBuffer.empty())
			{
				Graphics::GetInst()->GetDeviceContext()->Draw(m_vecMeshContainer[i].m_iCount, 0);
			}
		}
	}

	void Mesh::DrawContainer(int iIndex)
	{
		if (iIndex < 0 || iIndex >= static_cast<int>(m_vecMeshContainer.size()))
			return;

		MESHCONTAINER& container = m_vecMeshContainer[iIndex];

		UINT iStride = container.m_iSize;
		UINT iOffset = 0;

		Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 1, container.m_pVertexBuffer.GetAddressof(), &iStride, &iOffset);

		for (size_t j = 0; j < container.m_vecIndexBuffer.size(); ++j)
		{
			Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(*container.m_vecIndexBuffer[j].pBuffer, container.m_vecIndexBuffer[j].eFormat, 0);

			if (container.m_vecIndexBuffer[j].pBuffer)
			{
				Graphics::GetInst()->GetDeviceContext()->DrawIndexed(container.m_vecIndexBuffer[j].iCount, 0, 0);
			}
			else
			{
				Graphics::GetInst()->GetDeviceContext()->Draw(container.m_iCount, 0);
			}
		}

		if (container.m_vecIndexBuffer.empty())
		{
			Graphics::GetInst()->GetDeviceContext()->Draw(container.m_iCount, 0);
		}
	}

	void Mesh::DrawInst(int iCount, int iSize, const CPtr<ID3D11Buffer>& pInstBuffer, const MaterialResolver& resolver)
	{
		for (int i = 0; i < static_cast<int>(m_vecMeshContainer.size()); ++i)
		{
#ifdef _DEBUG
			if (!m_vecMeshContainer[i].bEnable)
			{
				continue;
			}
#endif
			// See Mesh::Draw — Material::Bind handles texture binding now.

			UINT iStrides[] = { static_cast<unsigned int>(m_vecMeshContainer[i].m_iSize), static_cast<unsigned int>(iSize) };
			UINT iOffsets[2] = {};

			ID3D11Buffer* pBuffers[] = { m_vecMeshContainer[i].m_pVertexBuffer.Get(), pInstBuffer.Get() };

			Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 2, pBuffers, iStrides, iOffsets);

			for (size_t j = 0; j < m_vecMeshContainer[i].m_vecIndexBuffer.size(); ++j)
			{
				std::shared_ptr<Material> pMat;
				if (resolver) pMat = resolver(i, static_cast<int>(j));
				if (!pMat && m_vecMeshContainer[i].vecMaterial.size() > j)
					pMat = m_vecMeshContainer[i].vecMaterial[j];
				if (pMat) pMat->Bind();

				Graphics::GetInst()->GetDeviceContext()->IASetIndexBuffer(m_vecMeshContainer[i].m_vecIndexBuffer[j].pBuffer.Get(), m_vecMeshContainer[i].m_vecIndexBuffer[j].eFormat, 0);

				if (m_vecMeshContainer[i].m_vecIndexBuffer[j].pBuffer)
				{
					Graphics::GetInst()->GetDeviceContext()->DrawIndexedInstanced(m_vecMeshContainer[i].m_vecIndexBuffer[j].iCount, static_cast<UINT>(iCount), 0, 0, 0);
				}
				else
				{
					Graphics::GetInst()->GetDeviceContext()->DrawInstanced(m_vecMeshContainer[i].m_iCount, static_cast<UINT>(iCount), 0, 0);
				}
			}
		}
	}

	std::shared_ptr<Bindable> Mesh::Clone()
	{
		return std::make_shared<Mesh>(*this);
	}

	// File magic + format-version progression:
	//   no magic, raw int container_count   = legacy ("v0")
	//   magic + version 1                   = materials/textures embedded
	//   magic + version 2                   = materials referenced by tag,
	//                                          live as standalone .mat assets
	// kMeshMagic decodes to a comically large container count if read raw,
	// which is why v0 detection is unambiguous.
	static constexpr uint32_t kMeshMagic   = 0x4853454D;  // 'MESH' (LE)
	static constexpr uint32_t kMeshVersion = 2;

	void Mesh::Save(FILE* pFile)
	{
		// v2 format (current):
		//   uint32 magic ('MESH')
		//   uint32 version (=2)
		//   int    container_count
		//   per container:
		//     int     vertex_count
		//     bytes   vertex data (m_iSize * vertex_count)
		//     int16   index_buffer_count
		//     per index buffer: int sub_count + uint32 indices[]
		//     int     material_count
		//     per material: int tag_length + bytes tag    ← reference only
		//
		// Material data itself lives in Resource/Material/*.mat and is loaded
		// at startup into BindableManager<Material>. v1 (embedded materials)
		// and v0 (legacy) keep loading via Mesh::Load's version branches.
		fwrite(&kMeshMagic, 4, 1, pFile);
		fwrite(&kMeshVersion, 4, 1, pFile);

		int iContainerCount = static_cast<int>(m_vecMeshContainer.size());
		fwrite(&iContainerCount, 4, 1, pFile);

		for (int i = 0; i < iContainerCount; ++i)
		{
			const MESHCONTAINER& container = m_vecMeshContainer[i];

			int iVertexCount = container.m_iCount;
			fwrite(&iVertexCount, 4, 1, pFile);
			if (iVertexCount > 0 && !container.m_vecCPUVertex.empty())
			{
				fwrite(container.m_vecCPUVertex.data(), 1, container.m_vecCPUVertex.size(), pFile);
			}

			short iIndexCount = static_cast<short>(container.m_vecCPUIndex.size());
			fwrite(&iIndexCount, 2, 1, pFile);

			for (short j = 0; j < iIndexCount; ++j)
			{
				int iSubCount = static_cast<int>(container.m_vecCPUIndex[j].size());
				fwrite(&iSubCount, 4, 1, pFile);

				if (iSubCount > 0)
				{
					fwrite(container.m_vecCPUIndex[j].data(), 4, iSubCount, pFile);
				}
			}

			int iMaterialCount = static_cast<int>(container.vecMaterial.size());
			fwrite(&iMaterialCount, 4, 1, pFile);
			for (int j = 0; j < iMaterialCount; ++j)
			{
				// Reference by tag. An empty tag (anonymous material) writes
				// length=0 — Load will fall back to a default Material then.
				const std::string& strTag = container.vecMaterial[j] ? container.vecMaterial[j]->GetTag() : std::string();
				int iLen = static_cast<int>(strTag.length());
				fwrite(&iLen, 4, 1, pFile);
				if (iLen) fwrite(strTag.c_str(), 1, iLen, pFile);
			}
		}
	}

	namespace
	{
		// Old-format helper: resolve a Texture loaded from disk against the
		// BindableManager cache, falling back to disk read if the manager
		// doesn't know it yet. Mirrors what the old Mesh::Load did inline.
		std::shared_ptr<Texture> ResolveLoadedTexture(const std::shared_ptr<Texture>& pTex)
		{
			if (auto pPrev = BindableManager<Texture>::GetInst()->FindBindable(pTex->GetTag()))
				return pPrev;
			if (pTex->LoadTextureFromFullPath(pTex->GetFullPath()))
				BindableManager<Texture>::GetInst()->AddBindable(pTex);
			return pTex;
		}

		// Dedup a freshly-loaded Material against BindableManager<Material>.
		// If a same-tag asset already exists in the cache, return that and
		// drop the just-read instance. Otherwise register the new one so
		// subsequent .mesh loads share it. Empty-tag materials skip the
		// cache (anonymous → per-mesh).
		std::shared_ptr<Material> DedupMaterial(const std::shared_ptr<Material>& pMat)
		{
			if (!pMat || pMat->GetTag().empty()) return pMat;
			auto* mgr = BindableManager<Material>::GetInst();
			if (auto pPrev = mgr->FindBindable(pMat->GetTag())) return pPrev;
			mgr->AddBindable(pMat);
			return pMat;
		}
	}

	void Mesh::Load(FILE* pFile)
	{
		// Three-way version detection:
		//   v0: no magic, raw int container_count (legacy)
		//   v1: magic + version 1, materials embedded
		//   v2: magic + version 2, materials referenced by tag
		uint32_t uMaybeMagic = 0;
		fread(&uMaybeMagic, 4, 1, pFile);

		uint32_t uVersion = 0;
		if (uMaybeMagic == kMeshMagic)
		{
			fread(&uVersion, 4, 1, pFile);
			assert((uVersion == 1 || uVersion == 2) && "Unknown .mesh version");
		}
		else
		{
			fseek(pFile, -4, SEEK_CUR);
			uVersion = 0;
		}

		int iContainerCount = 0;
		fread(&iContainerCount, 4, 1, pFile);

		std::vector<std::vector<VertexStandard>> _vecVertex;

		for (int i = 0; i < iContainerCount; ++i)
		{
			std::vector<VertexStandard> vecVertex;
			std::vector<std::vector<unsigned int>> vecIndex;

			int iVertexCount = 0;
			fread(&iVertexCount, 4, 1, pFile);
			vecVertex.resize(iVertexCount);
			fread(&vecVertex[0], sizeof(VertexStandard), iVertexCount, pFile);
			_vecVertex.push_back(vecVertex);

			short iIndexCount = 0;
			fread(&iIndexCount, 2, 1, pFile);
			vecIndex.resize(iIndexCount);
			for (short j = 0; j < iIndexCount; ++j)
			{
				int _iIndexCount = 0;
				fread(&_iIndexCount, 4, 1, pFile);
				if (_iIndexCount)
				{
					vecIndex[j].resize(_iIndexCount);
					fread(&vecIndex[j][0], 4, _iIndexCount, pFile);
				}
			}

			CreateMesh(vecVertex, vecIndex);

			// v0 only: container-level texture block precedes materials.
			// Stash for slot assignment after materials are read.
			std::vector<std::shared_ptr<Texture>> vecOldFormatTextures;
			if (uVersion == 0)
			{
				int iTextureCount = 0;
				fread(&iTextureCount, 4, 1, pFile);
				vecOldFormatTextures.reserve(iTextureCount);

				for (int j = 0; j < iTextureCount; ++j)
				{
					auto pTexture = std::make_shared<Texture>();
					pTexture->Load(pFile);
					pTexture = ResolveLoadedTexture(pTexture);
					vecOldFormatTextures.push_back(pTexture);
				}
			}

			int iMaterial = 0;
			fread(&iMaterial, 1, 4, pFile);

			for (int j = 0; j < iMaterial; ++j)
			{
				std::shared_ptr<Material> pMaterial;

				if (uVersion == 2)
				{
					// Reference-by-tag. Resolve against the global Material
					// asset cache (populated by ResourceManager::LoadAllMaterials
					// at startup). Missing tag → instantiate a default placeholder
					// so rendering doesn't crash; user can author the .mat later.
					int iLen = 0;
					fread(&iLen, 4, 1, pFile);
					std::string strTag;
					if (iLen)
					{
						std::unique_ptr<char[]> buf = std::make_unique<char[]>(iLen + 1);
						fread(buf.get(), 1, iLen, pFile);
						buf[iLen] = 0;
						strTag = buf.get();
					}

					if (!strTag.empty())
					{
						pMaterial = BindableManager<Material>::GetInst()->FindBindable(strTag);
						if (!pMaterial)
						{
							pMaterial = std::make_shared<Material>();
							pMaterial->SetTag(strTag);
							BindableManager<Material>::GetInst()->AddBindable(pMaterial);
						}
					}
					else
					{
						pMaterial = std::make_shared<Material>();
					}
				}
				else
				{
					// v0/v1: materials embedded in the .mesh. v1 has the
					// per-slot texture block; v0's LoadLegacy stops after
					// the 80-byte struct so file offsets align.
					pMaterial = std::make_shared<Material>();
					if (uVersion == 1) pMaterial->Load(pFile);
					else               pMaterial->LoadLegacy(pFile);

					// Dedup: same-tag materials across the mesh (or across
					// .mesh files loaded earlier) collapse to one cached
					// instance. This is what enables one-edit-affects-all
					// for repeated materials in a 200-container model.
					pMaterial = DedupMaterial(pMaterial);
				}

				AddMaterial(i, pMaterial);
			}

			// v0 fallout: distribute the container's loose texture list to
			// each material's slot array (keyed by t-register → slot index).
			if (uVersion == 0)
			{
				for (const auto& pTex : vecOldFormatTextures)
				{
					if (!pTex) continue;
					int iSlotIdx = Material::SlotRegisterToIndex(pTex->GetSlot());
					if (iSlotIdx < 0) continue;
					for (auto& pMat : m_vecMeshContainer[i].vecMaterial)
					{
						if (pMat) pMat->SetTexture(iSlotIdx, pTex);
					}
				}
			}
		}

		// Phase E7 — Drawable's StaticGetBoundingSphere migrated to
		// MeshUtils::ComputeBoundingSphere; same math, no Drawable dep.
		m_vBoundingSphereInfo = MeshUtils::ComputeBoundingSphere(_vecVertex);
	}
}
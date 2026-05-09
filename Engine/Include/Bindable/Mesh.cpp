#include "Mesh.h"
#include "Animation.h"
#include "MeshUtils.h"
#include "../Core/Graphics.h"

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
		for (size_t i = 0; i < m_vecMeshContainer.size(); ++i)
		{
			for (size_t j = 0; j < m_vecMeshContainer[i].vecMaterial.size(); ++j)
			{
				m_vecMeshContainer[i].vecMaterial[j] = m_vecMeshContainer[i].vecMaterial[j] ? std::static_pointer_cast<Material>(m_vecMeshContainer[i].vecMaterial[j]->Clone()) : nullptr;
			}
		}
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

	void Mesh::SetTextures(const std::vector<std::vector<std::shared_ptr<Texture>>>& vecTexture)
	{
		assert(m_vecMeshContainer.size() <= vecTexture.size());

		for (int i = 0; i < m_vecMeshContainer.size(); ++i)
		{
			m_vecMeshContainer[i].vecTexture = vecTexture[i];
		}
	}

	void Mesh::SetTextures(int iIndex, const std::vector<std::shared_ptr<Texture>>& vecTexture)
	{
		if (m_vecMeshContainer.size() <= iIndex || iIndex < 0)
		{
			assert(false);
			return;
		}

		m_vecMeshContainer[iIndex].vecTexture = vecTexture;
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

	void Mesh::Draw()
	{
		for (int i = 0; i < static_cast<int>(m_vecMeshContainer.size()); ++i)
		{
#ifdef _DEBUG
			if (!m_vecMeshContainer[i].bEnable)
			{
				continue;
			}
#endif

			for (size_t j = 0; j < m_vecMeshContainer[i].vecTexture.size(); ++j)
			{
				m_vecMeshContainer[i].vecTexture[j]->Bind();
			}

			UINT iStride = m_vecMeshContainer[i].m_iSize;
			UINT iOffset = 0;

			Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 1, m_vecMeshContainer[i].m_pVertexBuffer.GetAddressof(), &iStride, &iOffset);

			for (int j = 0; j < m_vecMeshContainer[i].m_vecIndexBuffer.size(); ++j)
			{
				if (m_vecMeshContainer[i].vecMaterial.size() > j && m_vecMeshContainer[i].vecMaterial[j])
				{
					m_vecMeshContainer[i].vecMaterial[j]->Bind();
				}

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

	void Mesh::DrawInst(int iCount, int iSize, const CPtr<ID3D11Buffer>& pInstBuffer)
	{
		for (int i = 0; i < static_cast<int>(m_vecMeshContainer.size()); ++i)
		{
#ifdef _DEBUG
			if (!m_vecMeshContainer[i].bEnable)
			{
				continue;
			}
#endif
			for (size_t j = 0; j < m_vecMeshContainer[i].vecTexture.size(); ++j)
			{
				m_vecMeshContainer[i].vecTexture[j]->Bind();
			}

			UINT iStrides[] = { static_cast<unsigned int>(m_vecMeshContainer[i].m_iSize), static_cast<unsigned int>(iSize) };
			UINT iOffsets[2] = {};

			ID3D11Buffer* pBuffers[] = { m_vecMeshContainer[i].m_pVertexBuffer.Get(), pInstBuffer.Get() };

			Graphics::GetInst()->GetDeviceContext()->IASetVertexBuffers(0, 2, pBuffers, iStrides, iOffsets);

			for (size_t j = 0; j < m_vecMeshContainer[i].m_vecIndexBuffer.size(); ++j)
			{
				if (m_vecMeshContainer[i].vecMaterial.size() > j)
				{
					m_vecMeshContainer[i].vecMaterial[j]->Bind();
				}

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

	void Mesh::Load(FILE* pFile)
	{
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

			std::vector<std::shared_ptr<Texture>> vecTexture;

			int iTextureCount = 0;

			fread(&iTextureCount, 4, 1, pFile);

			for (int j = 0; j < iTextureCount; ++j)
			{
				std::shared_ptr<Texture> pTexture = std::make_shared<Texture>();

				pTexture->Load(pFile);

				vecTexture.push_back(pTexture);
			}

			int iMaterial = 0;

			fread(&iMaterial, 1, 4, pFile);

			for (int j = 0; j < iMaterial; ++j)
			{
				std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

				pMaterial->Load(pFile);

				AddMaterial(i, pMaterial);
			}

			SetTextures(i, vecTexture);
		}

		// Phase E7 — Drawable's StaticGetBoundingSphere migrated to
		// MeshUtils::ComputeBoundingSphere; same math, no Drawable dep.
		m_vBoundingSphereInfo = MeshUtils::ComputeBoundingSphere(_vecVertex);
	}
}
#include "Terrain.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "../Core/Graphics.h"
#include "ColliderMesh.h"
#include "InputLayout.h"
#include "Topology.h"
#include "../Input/Input.h"
#include "TransformBuffer.h"

namespace Engine
{
	Terrain::Terrain() :
		Drawable()
		, m_pMesh(nullptr)
		, m_pHeightMap(nullptr)
		, m_bEditting(false)
		, m_fEditRange(5.f)
	{
		FindAndAddBind<InputLayout>("Standard");
		FindAndAddBind<VertexShader>("anisotropic_microfacet VS_Terrain");
		FindAndAddBind<PixelShader>("anisotropic_microfacet PS_Terrain");
		FindAndAddBind<Topology>("TriangleList");
		std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

		if (pMaterial)
		{
			pMaterial = std::static_pointer_cast<Material>(pMaterial->Clone());

			AddChild(pMaterial);
		}

		m_pVSTerrainBuffer = StaticFindBindable<ConstantBuffer<TERRAINCBUFFER>>("Terrain");
		m_pPSTerrainBuffer = StaticFindBindable<ConstantBuffer<TERRAINCBUFFER>>("Terrain");

		AddChild(m_pVSTerrainBuffer);
		AddChild(m_pPSTerrainBuffer);
	}

	void Terrain::CreateTerrain(int iWidth, int iHeight)
	{
		m_tTerrainBuffer.m_iWidth = iWidth;
		m_tTerrainBuffer.m_iHeight = iHeight;

		CreateVertexAndIndex(m_vecVertex, m_vecIndex, iWidth, iHeight);

		SetNormals(m_vecVertex, m_vecIndex);

		SetTangent(m_vecVertex, m_vecIndex);

		m_pMesh = CreateBindable<Mesh>("mesh", m_vecVertex, m_vecIndex);

		GetBoundingSphere(m_vecVertex);

		CreateMeshCollider(m_vecVertex, m_vecIndex);
	}

	void Terrain::CreateTerrainTexture(const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>("TerrainTexture", vecFullPath, TEXTURE_PATH, 20));
	}

	void Terrain::CreateTerrainNormalTexture(const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>("TerrainTexture", vecFullPath, TEXTURE_PATH, 21));
	}

	void Terrain::CreateTerrainSpecularTexture(const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>("TerrainTexture", vecFullPath, TEXTURE_PATH, 22));
	}

	void Terrain::CreateTerrainEmissiveTexture(const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>("TerrainTexture", vecFullPath, TEXTURE_PATH, 23));
	}

	void Terrain::CreateBlendTerrainTexture(const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>("TerrainTexture", vecFullPath, TEXTURE_PATH, 24));

		m_tTerrainBuffer.m_iBlendCount = static_cast<int>(vecFullPath.size());
	}

	void Terrain::CreateHeightMap(const TCHAR* pFilePath)
	{
		m_pHeightMap = CreateBindable<Texture>("HeightMap", pFilePath, TEXTURE_PATH, 16, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);

		DirectX::ScratchImage* pImage = m_pHeightMap->GetImage();

		if (!pImage) {
			assert(false);
			return;
		}

		uint8_t* pPixels = pImage->GetPixels();

		if (!pPixels) {
			assert(false);
			return;
		}

		m_vecHeight.resize(pImage->GetPixelsSize() / 4);

		for (int i = 0; i < static_cast<int>(m_vecHeight.size()); ++i, pPixels += 4)
		{
			m_vecHeight[i] = *pPixels;
		}

		CreateTerrain(pImage->GetMetadata().width - 1, pImage->GetMetadata().height - 1);
	}

	void Terrain::SaveHeightMap(const TCHAR* pFilePath, const std::string& strPathKey)
	{
		DirectX::ScratchImage* pImage = m_pHeightMap->GetImage();

		uint8_t* pPixels = pImage->GetPixels();

		for (int i = 0; i < static_cast<int>(m_vecHeight.size()); ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				pPixels[i * 4 + j] = m_vecHeight[i];
			}

			pPixels[3] = UINT8_MAX;
		}

		m_pHeightMap->SaveTexture(pFilePath, strPathKey);
	}

	void Terrain::CreateMeshCollider(std::vector<VertexStandard>& vecVertex, std::vector<unsigned int>& vecIndex)
	{
		std::vector<float> vecPoint;
		std::vector<int> _vecIndex;

		for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
		{
			vecPoint.push_back(vecVertex[i].pos.x);
			vecPoint.push_back(m_vecHeight[i] / 25.5f);
			vecPoint.push_back(vecVertex[i].pos.z);
		}

		_vecIndex.resize(vecIndex.size());

		memcpy_s(&_vecIndex[0], 4 * _vecIndex.size(), &vecIndex[0], 4 * vecIndex.size());

		std::shared_ptr<ColliderMesh> pCollider = FindChild<ColliderMesh>();

		if (!pCollider) {
			pCollider = CreateBindable<ColliderMesh>("TerrainCollider", vecPoint, _vecIndex);
		}
		else {
			pCollider->SetInfo(vecPoint, _vecIndex);
		}
	}

	void Terrain::CreateVertexAndIndex(std::vector<VertexStandard>& vecVertex, std::vector<unsigned int>& vecIndex, int iWidth, int iHeight)
	{
		for (int j = iHeight; j >= 0; --j)
		{
			for (int i = 0; i < iWidth + 1; ++i)
			{
				unsigned int iIndex = static_cast<unsigned int>(vecVertex.size());

				VertexStandard tVertex = {};

				tVertex.pos.x = static_cast<float>(i);
				tVertex.pos.z = static_cast<float>(j);

				tVertex.uv.x = static_cast<float>(i);
				tVertex.uv.y = static_cast<float>(j);

				vecVertex.push_back(tVertex);
			}
		}

		for (int i = 0; i < iWidth; ++i)
		{
			for (int j = 0; j < iHeight; ++j)
			{
				vecIndex.push_back(i + j * (iWidth + 1));
				vecIndex.push_back(i + 1 + j * (iWidth + 1));
				vecIndex.push_back(i + (j + 1) * (iWidth + 1));

				vecIndex.push_back(i + (j + 1) * (iWidth + 1));
				vecIndex.push_back(i + 1 + j * (iWidth + 1));
				vecIndex.push_back(i + 1 + (j + 1) * (iWidth + 1));
			}
		}
	}

	void Terrain::Bind()
	{
		m_pVSTerrainBuffer->UpdateBuffer(m_tTerrainBuffer);
		m_pPSTerrainBuffer->UpdateBuffer(m_tTerrainBuffer);

		__super::Bind();
	}
	void Terrain::CollisionStay(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		if (m_bEditting && CInput::GetInst()->IsMouseButtonPress(CInput::MOUSE_TYPE::LEFT)) {
			Vector3 vCross = pSrc->GetCross();

			std::shared_ptr<Transform> pTransform = GetTransform();

			vCross -= pTransform->GetPosition();

			vCross = pTransform->GetRotationMatrix().Transpose().TransformNormal(vCross);

			const Vector3 vScale = pTransform->GetScale();

			vCross.x /= vScale.x;
			vCross.y /= vScale.y;
			vCross.z /= vScale.z;


			int iMinX = static_cast<int>(vCross.x - m_fEditRange);

			iMinX = iMinX < 0 ? 0 : iMinX;

			int iMaxX = static_cast<int>(vCross.x + m_fEditRange);

			iMaxX = iMaxX >= m_tTerrainBuffer.m_iWidth ? m_tTerrainBuffer.m_iWidth : iMaxX;

			int iMinY = m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z + m_fEditRange) - 1;

			iMinY = iMinY < 0 ? 0 : iMinY;

			int iMaxY = m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z - m_fEditRange) - 1;

			iMaxY = iMaxY >= m_tTerrainBuffer.m_iWidth ? m_tTerrainBuffer.m_iWidth : iMaxY;

			DirectX::ScratchImage* pImage = m_pHeightMap->GetImage();

			if (!pImage) {
				return;
			}

			uint8_t* pPixel = pImage->GetPixels();

			if (!pPixel) {
				return;
			}

			for (int i = iMinX; i <= iMaxX; ++i)
			{
				for (int j = iMinY; j <= iMaxY; ++j)
				{
					float fDist = sqrtf((vCross.x - i) * (vCross.x - i) + (m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z) - 1 - j) * (m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z) - 1 - j));

					if (fDist > m_fEditRange) {
						continue;
					}

					int iIndex = i + j * (m_tTerrainBuffer.m_iWidth + 1);

					int iHeight = m_vecHeight[iIndex];

					m_vecHeight[iIndex] = iHeight + 1;

					if (m_vecHeight[iIndex] > UINT8_MAX) {
						m_vecHeight[iIndex] = UINT8_MAX;
					}

					pPixel[iIndex * 4] = m_vecHeight[iIndex];
					pPixel[iIndex * 4 + 1] = m_vecHeight[iIndex];
					pPixel[iIndex * 4 + 2] = m_vecHeight[iIndex];
				}
			}

			m_pHeightMap->CreateShaderResourceView(*pImage);

			CreateMeshCollider(m_vecVertex, m_vecIndex);
		}

		else if (CInput::GetInst()->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT)) {
			m_bEditting = true;
		}
	}
	void Terrain::CollisionEnd(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		if (m_bEditting) {
			m_bEditting = false;
		}
	}
}
#include "Terrain.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "../Core/Graphics.h"
#include "ColliderMesh.h"
#include "InputLayout.h"
#include "Topology.h"

namespace Engine
{
	Terrain::Terrain() :
		Drawable()
		, m_pMesh(nullptr)
		, m_pHeightMap(nullptr)
	{
		CreateTerrain(100, 100);

		m_tTerrainBuffer.m_iWidth = 100;
		m_tTerrainBuffer.m_iHeight = 100;

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

		m_pVSTerrainBuffer = StaticFindBindable<VertexCBuffer<TERRAINCBUFFER>>("Terrain");
		m_pPSTerrainBuffer = StaticFindBindable<PixelCBuffer<TERRAINCBUFFER>>("Terrain");

		AddChild(m_pVSTerrainBuffer);
		AddChild(m_pPSTerrainBuffer);
	}

	void Terrain::CreateTerrain(int iWidth, int iHeight)
	{
		std::vector<VertexStandard> vecVertex;

		std::vector<unsigned int> vecIndex;

		for (int j = iHeight - 1; j >= 0; --j)
		{
			for (int i = 0; i < iWidth; ++i)
			{
				unsigned int iIndex = static_cast<unsigned int>(vecVertex.size());

				for (int k = 0; k < 4; ++k)
				{
					VertexStandard tVertex = {};

					tVertex.pos.x = static_cast<float>(i + k % 2);
					tVertex.pos.z = static_cast<float>(j + 1 - k / 2);

					tVertex.uv.x = static_cast<float>(k % 2);
					tVertex.uv.y = static_cast<float>(k / 2);

					vecVertex.push_back(tVertex);
				}

				vecIndex.push_back(iIndex);
				vecIndex.push_back(iIndex + 1);
				vecIndex.push_back(iIndex + 2);

				vecIndex.push_back(iIndex + 1);
				vecIndex.push_back(iIndex + 3);
				vecIndex.push_back(iIndex + 2);
			}
		}

		SetNormals(vecVertex, vecIndex);

		SetTangent(vecVertex, vecIndex);

		m_pMesh = CreateBindable<Mesh>("mesh", vecVertex, vecIndex);

		GetBoundingSphere(vecVertex);
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
	}

	void Terrain::Bind()
	{
		m_pVSTerrainBuffer->UpdateBuffer(m_tTerrainBuffer);
		m_pPSTerrainBuffer->UpdateBuffer(m_tTerrainBuffer);

		__super::Bind();
	}
}
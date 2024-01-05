#include "Terrain.h"
#include "Mesh.h"
#include "BindableManager.h"
#include "../Core/Graphics.h"
#include "ColliderMesh.h"
#include "InputLayout.h"
#include "Topology.h"
#include "../Input/Input.h"
#include "Transform.h"
#include "Decal.h"
#include "../Shader/StructuredBuffer.h"

namespace Engine
{
	Terrain::Terrain() :
		Drawable()
		, m_pMesh(nullptr)
		, m_pTerrainCBuffer(StaticFindBindable<ConstantBuffer<TERRAINCBUFFER>>("Terrain"))
		, m_pHeightMap(nullptr)
		, m_bEditting(false)
		, m_fEditRange(5.f)
		, m_pDecal()
		, m_bEraseMode(false)
	{
		SetBindableType(BINDABLE_TYPE::TERRAIN);
	}

	void Terrain::CreateTerrain(int iWidth, int iHeight)
	{
		m_tTerrainBuffer.m_iWidth = iWidth;
		m_tTerrainBuffer.m_iHeight = iHeight;

		CreateTileTypeBuffer(iWidth, iHeight);

		CreateVertexAndIndex(m_vecVertex, m_vecIndex, iWidth, iHeight);

		SetNormals(m_vecVertex, m_vecIndex);

		SetTangent(m_vecVertex, m_vecIndex);

		m_pMesh = CreateBindable<Mesh>("terrainmesh", m_vecVertex, m_vecIndex);

		GetBoundingSphere(m_vecVertex);

		CreateMeshCollider();
	}

	void Terrain::CreateTileTypeBuffer(int iWidth, int iHeight)
	{
		m_pTileTypeBuffer = std::make_shared<StructuredBuffer>(iWidth * iHeight, 4);

		m_vecTileType.resize(iWidth * iHeight);
	}

	void Terrain::CreateTerrainTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>(strTag, vecFullPath, TEXTURE_PATH, 20));
	}

	void Terrain::CreateTerrainNormalTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>(strTag, vecFullPath, TEXTURE_PATH, 21));
	}

	void Terrain::CreateTerrainSpecularTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>(strTag, vecFullPath, TEXTURE_PATH, 22));
	}

	void Terrain::CreateTerrainEmissiveTexture(const std::string& strTag, const std::vector<const TCHAR*>& vecFullPath)
	{
		m_vecTexture.push_back(CreateBindable<Texture>(strTag, vecFullPath, TEXTURE_PATH, 23));
	}

	void Terrain::CreateHeightMap(const std::string& strTag, const TCHAR* pFilePath)
	{
		m_pHeightMap = CreateBindable<Texture>(strTag, pFilePath, TEXTURE_PATH, 16, D3D11_CPU_ACCESS_WRITE, D3D11_USAGE_DYNAMIC);

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

		CreateTerrain(static_cast<int>(pImage->GetMetadata().width) - 1, static_cast<int>(pImage->GetMetadata().height) - 1);
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

	void Terrain::CreateMeshCollider()
	{
		std::vector<float> vecPoint;
		std::vector<int> _vecIndex;

		GetPoints(vecPoint);

		GetTris(_vecIndex);

		std::shared_ptr<ColliderMesh> pCollider = FindChild<ColliderMesh>();

		if (!pCollider) {
			pCollider = CreateBindable<ColliderMesh>("TerrainCollider", vecPoint, _vecIndex);
		}
		else {
			pCollider->SetInfo(vecPoint, _vecIndex);
		}

		pCollider->SetColliderType(COLLIDER_TYPE::TERRAIN);
	}

	void Terrain::GetPoints(std::vector<float>& vecPoint)
	{
		vecPoint.clear();

		for (int i = 0; i < static_cast<int>(m_vecVertex.size()); ++i)
		{
			vecPoint.push_back(m_vecVertex[i].pos.x);
			vecPoint.push_back(static_cast<float>(m_vecHeight[i]));
			vecPoint.push_back(m_vecVertex[i].pos.z);
		}
	}

	void Terrain::GetTris(std::vector<int>& _vecIndex)
	{
		_vecIndex.resize(m_vecIndex.size());

		memcpy_s(&_vecIndex[0], 4 * _vecIndex.size(), &m_vecIndex[0], 4 * m_vecIndex.size());
	}

	void Terrain::SetEraseMode()
	{
		m_bEraseMode = true;
	}

	void Terrain::SetAddMode()
	{
		m_bEraseMode = false;
	}

	bool Terrain::IsEraseMode() const
	{
		return m_bEraseMode;
	}

	float Terrain::GetTerrainHeight(const Vector3& vPos)
	{
		const Vector3& vLocalPos = GetTerrainLocalPos(vPos);

		int iLeftTopIndex = GetTerrainIndex(vLocalPos);

		if (iLeftTopIndex == -1)
		{
			return 0.f;
		}

		int iRightTopIndex = iLeftTopIndex + 1;
		int iLeftBottomIndex = iLeftTopIndex + m_tTerrainBuffer.m_iWidth + 1;
		int iRightBottomIndex = iLeftBottomIndex + 1;

		if (iLeftTopIndex >= m_vecVertex.size() ||
			iRightTopIndex >= m_vecVertex.size() ||
			iLeftBottomIndex >= m_vecVertex.size() ||
			iRightBottomIndex >= m_vecVertex.size())
		{
			return 0.f;
		}

		if (vLocalPos.x - static_cast<int>(vLocalPos.x) < vLocalPos.z - static_cast<int>(vLocalPos.z))
		{
			float s = 1.f - vLocalPos.z + static_cast<int>(vLocalPos.z);
			float t = vLocalPos.x - static_cast<int>(vLocalPos.x);

			Vector3 u = Vector3(m_vecVertex[iLeftBottomIndex].pos.x, static_cast<float>(m_vecHeight[iLeftBottomIndex]), m_vecVertex[iLeftBottomIndex].pos.z) -
			Vector3(m_vecVertex[iLeftTopIndex].pos.x, static_cast<float>(m_vecHeight[iLeftTopIndex]), m_vecVertex[iLeftTopIndex].pos.z);

			Vector3 v = Vector3(m_vecVertex[iRightTopIndex].pos.x, static_cast<float>(m_vecHeight[iRightTopIndex]), m_vecVertex[iRightTopIndex].pos.z) -
				Vector3(m_vecVertex[iLeftTopIndex].pos.x, static_cast<float>(m_vecHeight[iLeftTopIndex]), m_vecVertex[iLeftTopIndex].pos.z);

			Vector3 vResult = (u * s + v * t) + Vector3(m_vecVertex[iLeftTopIndex].pos.x, static_cast<float>(m_vecHeight[iLeftTopIndex]), m_vecVertex[iLeftTopIndex].pos.z);

			return GetTransform()->GetTransformMatrix().TransformCoord(vResult).y;
		}
		else
		{
			float s = vLocalPos.z - static_cast<int>(vLocalPos.z);
			float t = 1.f - vLocalPos.x + static_cast<int>(vLocalPos.x);
			Vector3 u = Vector3(m_vecVertex[iRightTopIndex].pos.x, static_cast<float>(m_vecHeight[iRightTopIndex]), m_vecVertex[iRightTopIndex].pos.z) -
			Vector3(m_vecVertex[iRightBottomIndex].pos.x, static_cast<float>(m_vecHeight[iRightBottomIndex]), m_vecVertex[iRightBottomIndex].pos.z);

			Vector3 v = Vector3(m_vecVertex[iLeftBottomIndex].pos.x, static_cast<float>(m_vecHeight[iLeftBottomIndex]), m_vecVertex[iLeftBottomIndex].pos.z) -
				Vector3(m_vecVertex[iRightBottomIndex].pos.x, static_cast<float>(m_vecHeight[iRightBottomIndex]), m_vecVertex[iRightBottomIndex].pos.z);

			Vector3 vResult = (u * s + v * t) + Vector3(m_vecVertex[iRightBottomIndex].pos.x, static_cast<float>(m_vecHeight[iRightBottomIndex]), m_vecVertex[iRightBottomIndex].pos.z);

			return GetTransform()->GetTransformMatrix().TransformCoord(vResult).y;
		}

		return 0.0f;
	}

	void Terrain::AddTerrainHeight(const Vector3& vPos, int iHeight)
	{
		int iLeftTopIndex = GetTerrainIndex(GetTerrainLocalPos(vPos));
		
		if (iLeftTopIndex == -1)
		{
			return;
		}

		int iRightTopIndex = iLeftTopIndex + 1;
		int iLeftBottonIndex = iLeftTopIndex + m_tTerrainBuffer.m_iWidth + 1;
		int iRightBottomIndex = iLeftBottonIndex + 1;

		int iMin = std::min(std::min(m_vecHeight[iLeftTopIndex], m_vecHeight[iRightTopIndex]), std::min(m_vecHeight[iLeftBottonIndex], m_vecHeight[iRightBottomIndex]));

		if (iMin == m_vecHeight[iLeftTopIndex])
		{
			m_vecHeight[iLeftTopIndex] += iHeight;
		}
		if (iMin == m_vecHeight[iRightTopIndex])
		{
			m_vecHeight[iRightTopIndex] += iHeight;
		}
		if (iMin == m_vecHeight[iLeftBottonIndex])
		{
			m_vecHeight[iLeftBottonIndex] += iHeight;
		}
		if (iMin == m_vecHeight[iRightBottomIndex])
		{
			m_vecHeight[iRightBottomIndex] += iHeight;
		}

		DirectX::ScratchImage* pImage = m_pHeightMap->GetImage();

		if (!pImage)
		{
			assert(false);
			return;
		}

		uint8_t* pPixels = pImage->GetPixels();

		if (!pPixels)
		{
			assert(false);
			return;
		}

		memset(&pPixels[iLeftTopIndex * 4], m_vecHeight[iLeftTopIndex], 4);
		memset(&pPixels[iRightTopIndex * 4], m_vecHeight[iRightTopIndex], 4);
		memset(&pPixels[iLeftBottonIndex * 4], m_vecHeight[iLeftBottonIndex], 4);
		memset(&pPixels[iRightBottomIndex * 4], m_vecHeight[iRightBottomIndex], 4);

		m_pHeightMap->CreateTexture(*pImage);

		m_pHeightMap->CreateShaderResourceView(pImage->GetMetadata().format, static_cast<int>(pImage->GetMetadata().mipLevels), static_cast<int>(pImage->GetMetadata().arraySize));

		CreateMeshCollider();
	}

	int Terrain::GetTerrainIndex(const Vector3& vLocalPos) const
	{
		if (vLocalPos.x < 0.f || vLocalPos.z < 0.f ||
			vLocalPos.x >= m_tTerrainBuffer.m_iWidth ||
			vLocalPos.z >= m_tTerrainBuffer.m_iHeight)
		{
			return -1;
		}

		return static_cast<int>(floorf(vLocalPos.x)) + (m_tTerrainBuffer.m_iHeight - static_cast<int>(ceilf(vLocalPos.z))) * (m_tTerrainBuffer.m_iWidth + 1);
	}

	Vector3 Terrain::GetTerrainLocalPos(const Vector3& vPos)
	{
		std::shared_ptr<Transform> pTransform = GetTransform();

		return (Matrix::TranslateFromVector(-pTransform->GetPosition()) * pTransform->GetRotationMatrix().Transpose() * Matrix::Scaling(1.f / pTransform->GetScale())).TransformCoord(vPos);
	}

	void Terrain::SetTileType(const Vector3& vWorldPos, int iTileType)
	{
		const Vector3& vLocalPos = GetTerrainLocalPos(vWorldPos);

		if (vLocalPos.x >= m_tTerrainBuffer.m_iWidth ||
			vLocalPos.z >= m_tTerrainBuffer.m_iHeight ||
			vLocalPos.x < 0.f || vLocalPos.z < 0.f)
		{
			return;
		}

		int iIndex = static_cast<int>(vLocalPos.x) + static_cast<int>(m_tTerrainBuffer.m_iHeight - vLocalPos.z) * m_tTerrainBuffer.m_iWidth;

		m_vecTileType[iIndex] = iTileType;

		m_pTileTypeBuffer->WriteData(&m_vecTileType[0], 0, 4 * static_cast<int>(m_vecTileType.size()));
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

		for (int j = 0; j < iHeight; ++j)
		{
			for (int i = 0; i < iWidth; ++i)
			{
				// 0 1 129
				vecIndex.push_back(i + j * (iWidth + 1));
				vecIndex.push_back(i + 1 + j * (iWidth + 1));
				vecIndex.push_back(i + (j + 1) * (iWidth + 1));

				// 129 1 130
				vecIndex.push_back(i + (j + 1) * (iWidth + 1));
				vecIndex.push_back(i + 1 + j * (iWidth + 1));
				vecIndex.push_back(i + 1 + (j + 1) * (iWidth + 1));
			}
		}
	}

	bool Terrain::Init()
	{
		if (!__super::Init())
		{
			return false;
		}

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

		m_pDecal = CreateBindable<Decal>("BrushDecal");

		m_pDecal->FindAndAddBind<Mesh>("Box");
		m_pDecal->FindAndAddBind<Topology>("TriangleList");

		return true;
	}

	void Terrain::Bind()
	{
		m_pTileTypeBuffer->SetSRV(24);

		m_pTerrainCBuffer->UpdateBuffer(m_tTerrainBuffer);

		m_pTerrainCBuffer->Bind();

		__super::Bind();

		m_pTileTypeBuffer->ResetSRV(24);
	}
	void Terrain::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_tTerrainBuffer, sizeof(TERRAINCBUFFER), 1, pFile);

		if (m_tTerrainBuffer.m_iWidth >= 0 && m_tTerrainBuffer.m_iHeight >= 0)
		{
			fwrite(&m_vecHeight[0], 4, (m_tTerrainBuffer.m_iWidth + 1) * (m_tTerrainBuffer.m_iHeight + 1), pFile);
			fwrite(&m_vecTileType[0], 4, m_tTerrainBuffer.m_iWidth * m_tTerrainBuffer.m_iHeight, pFile);
		}

		bool bHeightMap = static_cast<bool>(m_pHeightMap);

		fwrite(&bHeightMap, 1, 1, pFile);

		if (bHeightMap)
		{
			size_t iLength = m_pHeightMap->GetTag().length();

			fwrite(&iLength, 4, 1, pFile);

			fwrite(m_pHeightMap->GetTag().c_str(), 1, m_pHeightMap->GetTag().length(), pFile);
		}
	}
	void Terrain::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_tTerrainBuffer, sizeof(TERRAINCBUFFER), 1, pFile);

		if (m_tTerrainBuffer.m_iWidth >= 0 && m_tTerrainBuffer.m_iHeight >= 0)
		{
			m_vecHeight.resize((m_tTerrainBuffer.m_iWidth + 1)*  (m_tTerrainBuffer.m_iHeight + 1));

			fread(&m_vecHeight[0], 4, (m_tTerrainBuffer.m_iWidth + 1) * (m_tTerrainBuffer.m_iHeight + 1), pFile);

			m_vecTileType.resize(m_tTerrainBuffer.m_iWidth * m_tTerrainBuffer.m_iHeight);

			fread(&m_vecTileType[0], 4, m_tTerrainBuffer.m_iWidth * m_tTerrainBuffer.m_iHeight, pFile);
		}

		bool bHeightMap = false;

		fread(&bHeightMap, 1,1, pFile);

		if (bHeightMap)
		{
			int iLength = 0;

			fread(&iLength, 4, 1, pFile);
			
			std::unique_ptr<char[]> strTexture = std::make_unique<char[]>(iLength + 1);

			strTexture[iLength] = 0;

			fread(strTexture.get(), 1, iLength, pFile);

			m_pHeightMap = std::static_pointer_cast<Texture>(FindChild(strTexture.get()));
		}

		CreateTerrain(m_tTerrainBuffer.m_iWidth, m_tTerrainBuffer.m_iHeight);
	}
	void Terrain::CollisionStay(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		if (m_bEditting)
		{
			if (CInput::GetInst()->IsMouseButtonUp(CInput::MOUSE_TYPE::LEFT)) 
			{
				Vector3 vCross = pSrc->GetCross();

				std::shared_ptr<Transform> pTransform = GetTransform();

				vCross -= pTransform->GetPosition();

				vCross = pTransform->GetRotationMatrix().Transpose().TransformNormal(vCross);

				const Vector3 vScale = pTransform->GetScale();

				vCross.x /= vScale.x;
				vCross.y /= vScale.y;
				vCross.z /= vScale.z;

				EditHeightMapWithTexture(vCross);
			}
			else
			{

			}
		}

		else if (CInput::GetInst()->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT)) 
		{
			m_bEditting = true;
		}

		std::shared_ptr<Transform> pDecalTransform = m_pDecal->GetTransform();

		const Vector3& vCross = pSrc->GetCross();

		pDecalTransform->SetPosition(ceilf(vCross.x), vCross.y, ceilf(vCross.z));

		if (m_pBrushTexture)
		{
			m_pDecal->Enable();
		}
	}
	void Terrain::CollisionEnd(Collider* pSrc, Collider* pDest, float fDeltaTime)
	{
		if (m_bEditting) 
		{
			m_bEditting = false;
		}

		m_pDecal->Disable();
	}

	void Terrain::EditHeightMapWithTexture(const Vector3& vCross)
	{
		if (!m_pBrushTexture)
		{
			return;
		}

		int iBrushWidth = m_pBrushTexture->GetImageWidth();

		int iBrushHeight = m_pBrushTexture->GetImageHeight();

		int iMinX = static_cast<int>(vCross.x - iBrushWidth / 2);

		iMinX = iMinX < 0 ? 0 : iMinX;

		int iMaxX = iMinX + iBrushWidth - 1;

		iMaxX = iMaxX >= m_tTerrainBuffer.m_iWidth ? m_tTerrainBuffer.m_iWidth : iMaxX;

		int iMinY = m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z + iBrushHeight / 2) - 1;

		iMinY = iMinY < 0 ? 0 : iMinY;

		int iMaxY = iMinY + iBrushHeight - 1;

		iMaxY = iMaxY >= m_tTerrainBuffer.m_iWidth ? m_tTerrainBuffer.m_iWidth : iMaxY;

		DirectX::ScratchImage* pImage = m_pHeightMap->GetImage();

		if (!pImage)
		{
			return;
		}

		uint8_t* pPixel = pImage->GetPixels();

		if (!pPixel)
		{
			return;
		}

		DirectX::ScratchImage* pBrushImage = m_pBrushTexture->GetImage();

		if (!pBrushImage)
		{
			return;
		}

		uint8_t* pBrushPixel = pBrushImage->GetPixels();

		if (!pBrushPixel)
		{
			return;
		}

		for (int i = iMinX; i <= iMaxX; ++i)
		{
			for (int j = iMinY; j <= iMaxY; ++j)
			{
				int iIndex = i + j * (m_tTerrainBuffer.m_iWidth + 1);

				int iHeight = m_vecHeight[iIndex];

				int iBrushIndex = i - static_cast<int>(vCross.x - iBrushWidth / 2) + (iBrushHeight - (j - (m_tTerrainBuffer.m_iHeight - static_cast<int>(vCross.z + iBrushHeight / 2) - 1)) - 1) * iBrushWidth;

				int iBrushColor = pBrushPixel[iBrushIndex * 4];

				int iBrushAlpha = pBrushPixel[iBrushIndex * 4 + 3];

				int iNewHeight = iHeight + static_cast<int>(iBrushColor * (iBrushAlpha / 255.f)) * (1 - 2 * m_bEraseMode);

				m_vecHeight[iIndex] = iNewHeight;

				if (m_vecHeight[iIndex] > UINT8_MAX)
				{
					m_vecHeight[iIndex] = UINT8_MAX;
				}
				else if (m_vecHeight[iIndex] < 0)
				{
					m_vecHeight[iIndex] = 0;
				}

				pPixel[iIndex * 4] = m_vecHeight[iIndex];
				pPixel[iIndex * 4 + 1] = m_vecHeight[iIndex];
				pPixel[iIndex * 4 + 2] = m_vecHeight[iIndex];
			}
		}

		m_pHeightMap->CreateTexture(*pImage);

		m_pHeightMap->CreateShaderResourceView(pImage->GetMetadata().format, static_cast<int>(pImage->GetMetadata().mipLevels), static_cast<int>(pImage->GetMetadata().arraySize));

		CreateMeshCollider();
	}
	void Terrain::SetBrushTexture(std::shared_ptr<Texture> pBrushTexture)
	{
		m_pBrushTexture = pBrushTexture;

		if (m_pBrushTexture)
		{
			std::shared_ptr<Bindable> pTexture = m_pDecal->FindChild(BINDABLE_TYPE::TEXTURE);

			if (pTexture)
			{
				m_pDecal->DeleteChild(pTexture);
			}

			m_pDecal->AddChild(m_pBrushTexture);

			std::shared_ptr<Transform> pDecalTransform = m_pDecal->GetTransform();

			int iWidth = m_pBrushTexture->GetImageWidth();

			int iHeight = m_pBrushTexture->GetImageHeight();

			pDecalTransform->SetScale(static_cast<float>(iWidth), (iWidth + iHeight) / 2.f, static_cast<float>(iHeight));
		}
	}
}

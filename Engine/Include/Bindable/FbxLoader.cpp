#include "FbxLoader.h"
#include "../Core/PathManager.h"
#include "../Vector4.h"
#include "MeshUtils.h"

namespace Engine
{
	FbxLoader::FbxLoader() :
		m_pManager(nullptr)
		, m_pScene(nullptr)
		, m_bCalculateNormal(false)
		, m_bCalculateTangent(false)
	{
	}

	FbxLoader::~FbxLoader()
	{
		if (m_pScene)
		{
			m_pScene->Destroy();
		}

		if (m_pManager)
		{
			m_pManager->Destroy();
		}
	}

	bool FbxLoader::Init()
	{
		m_pManager = FbxManager::Create();

		if (!m_pManager)
		{
			return false;
		}

		FbxIOSettings* ios = FbxIOSettings::Create(m_pManager, IOSROOT);

		assert(ios);

		m_pManager->SetIOSettings(ios);

		return true;
	}

	bool FbxLoader::LoadFile(const TCHAR* pFileName, const std::string& strPathKey)
	{
		if (!LoadScene(pFileName, strPathKey))
		{
			return false;
		}

		fbxsdk::FbxArray<fbxsdk::FbxString*> StringArray;

		m_pScene->FillAnimStackNameArray(StringArray);

		if (StringArray.GetCount())
		{
			LoadAnimationClip(StringArray);

			LoadBone(m_pScene->GetRootNode());

			for (int i = 0; i < static_cast<int>(m_vecSequence.size()); ++i)
			{
				m_vecSequence[i].vecBoneKeyFrame.resize(m_tSkeleton.vecBone.size());
			}
		}

		LoadScene(m_pScene->GetRootNode());

		LoadBindPose();

		LoadAnimation();

		for (int i = 0; i < static_cast<int>(m_vecSequence.size()); ++i)
		{
			for (int j = 0; j < m_vecSequence[i].vecBoneKeyFrame.size(); ++j)
			{
				for (int k = 0; k < m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame.size(); ++k)
				{
					if (m_tSkeleton.vecBone[j].iParent != -1)
					{
						m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame[k].matTransform =
							m_vecSequence[i].vecBoneKeyFrame[m_tSkeleton.vecBone[j].iParent].vecKeyFrame[k].matTransform * m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame[k].matTransform;
					}
				}
			}
		}


		for (int i = 0; i < static_cast<int>(m_vecSequence.size()); ++i)
		{
			for (int j = 0; j < m_vecSequence[i].vecBoneKeyFrame.size(); ++j)
			{
				fbxsdk::FbxAMatrix matBind;

				for (int k = 0; k < 4; ++k)
				{
					matBind.SetRow(k, m_vecBindPose[j].GetRow(k));
				}

				for (int k = 0; k < m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame.size(); ++k)
				{
					fbxsdk::FbxAMatrix matConvert;

					matConvert.SetRow(0, { 1.f, 0.f, 0.f, 0.f });
					matConvert.SetRow(1, { 0.f, 0.f, 1.f, 0.f });
					matConvert.SetRow(2, { 0.f, 1.f, 0.f, 0.f });
					matConvert.SetRow(3, { 0.f, 0.f, 0.f, 1.f });

					m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame[k].matTransform = 
						matConvert * m_vecSequence[i].vecBoneKeyFrame[j].vecKeyFrame[k].matTransform * matConvert.Inverse();
				}
			}
		}

		return true;
	}

	bool FbxLoader::LoadScene(const TCHAR* pFileName, const std::string& strPathKey)
	{
		TCHAR strFullPath[MAX_PATH] = {};

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		if (pPath)
		{
			wcscpy_s(strFullPath, pPath);
		}

		wcscat_s(strFullPath, pFileName);

		char pFullPath[MAX_PATH] = {};

#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, strFullPath, -1, pFullPath, MAX_PATH, nullptr, nullptr);
#else
		strcpy_s(pFullPath, strFullPath);
#endif

		m_pScene = FbxScene::Create(m_pManager, "Importer Scene");

		FbxImporter* pImporter = FbxImporter::Create(m_pManager, "Importer");

		if (!pImporter ||
			!pImporter->Initialize(pFullPath, -1, m_pManager->GetIOSettings()) ||
			!pImporter->Import(m_pScene))
		{
			return false;
		}

		m_fSceneScale = static_cast<float>(m_pScene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m));

		return true;
	}

	int FbxLoader::GetLODCount() const
	{
		return static_cast<int>(m_tScene.vecLODGroup.size());
	}

	std::vector<VertexStandard>& FbxLoader::GetVertexData(int iIndex)
	{
		return m_tScene.vecLODGroup[iIndex].tMesh.vecVertex;
	}

	std::string FbxLoader::GetMeshName(int iIndex) const
	{
		return m_tScene.vecLODGroup[iIndex].tMesh.name;
	}

	const std::vector<std::vector<unsigned int>>& FbxLoader::GetIndexData(int iIndex) const
	{
		return m_tScene.vecLODGroup[iIndex].tMesh.vecIndex;
	}

	bool FbxLoader::IsCalculatedTangent() const
	{
		return m_bCalculateTangent;
	}

	const std::vector<FbxLoader::TEXTUREINFO>& FbxLoader::GetTextures(int iIndex) const
	{
		return m_tScene.vecLODGroup[iIndex].tMesh.m_vecTextureInfo;
	}

	const std::vector<FbxLoader::MATERIALINFO>& FbxLoader::GetMaterials(int iIndex) const
	{
		return m_tScene.vecLODGroup[iIndex].tMesh.m_vecMaterial;
	}

	const std::vector<FbxLoader::SEQUENCE>& FbxLoader::GetSequences(int iIndex) const
	{
		return m_tScene.vecLODGroup[iIndex].vecSequence;
	}

	const std::vector<FbxLoader::SEQUENCE>& FbxLoader::GetSequences() const
	{
		return m_vecSequence;
	}

	const FbxLoader::SKELETON& FbxLoader::GetSkeleton(int iIndex) const
	{
		return m_tSkeleton;
	}

	fbxsdk::FbxAMatrix FbxLoader::GetTransform(fbxsdk::FbxNode* pNode)
	{
		fbxsdk::FbxVector4 vT = pNode->GetGeometricTranslation(fbxsdk::FbxNode::eSourcePivot);
		fbxsdk::FbxVector4 vR = pNode->GetGeometricRotation(fbxsdk::FbxNode::eSourcePivot);
		fbxsdk::FbxVector4 vS = pNode->GetGeometricScaling(fbxsdk::FbxNode::eSourcePivot);

		return FbxAMatrix(vT, vR, vS);
	}

	void FbxLoader::LoadScene(fbxsdk::FbxNode* pNode)
	{
		if (pNode->GetMesh())
		{
			LODGROUP group;

			group.vecSequence = m_vecSequence;

			LoadMesh(pNode, group);

			group.name = group.tMesh.name;

			m_tScene.vecLODGroup.push_back(group);
		}

		int iCount = pNode->GetChildCount();

		for (int i = 0; i < iCount; ++i)
		{
			LoadScene(pNode->GetChild(i));
		}
	}

	void FbxLoader::LoadMesh(fbxsdk::FbxNode* pNode, LODGROUP& vecMesh)
	{
		fbxsdk::FbxMesh* pMesh = pNode->GetMesh();

		if (!pMesh)
		{
			return;
		}

		if (pMesh->RemoveBadPolygons() < 0)
		{
			return;
		}

		FbxGeometryConverter gc = m_pManager;

		pMesh = static_cast<fbxsdk::FbxMesh*>(gc.Triangulate(pMesh, true));

		if (!pMesh ||
			pMesh->RemoveBadPolygons() < 0)
		{
			return;
		}

		vecMesh.tMesh.iLodId = 0;
		vecMesh.tMesh.fLodThreshold = -1.f;
		vecMesh.tMesh.name = pNode->GetName()[0] != '\0' ? pNode->GetName() : pMesh->GetName();

		while (true)
		{
			bool bCheck = true;

			for (size_t i = 0; i < m_tScene.vecLODGroup.size(); ++i)
			{
				if (m_tScene.vecLODGroup[i].name == vecMesh.tMesh.name) {
					vecMesh.tMesh.name = vecMesh.tMesh.name + "_2";
					bCheck = false;
					break;
				}
			}

			if (bCheck)
			{
				break;
			}
		}

		LoadMeshData(pMesh, vecMesh.tMesh, vecMesh);
	}

	bool FbxLoader::LoadMeshData(fbxsdk::FbxMesh* pMesh, MESH& mesh, LODGROUP& group)
	{
		int iPolygon = pMesh->GetPolygonCount();

		if (iPolygon <= 0)
		{
			return false;
		}

		int iVertexCount = pMesh->GetControlPointsCount();
		fbxsdk::FbxVector4* pVertexs = pMesh->GetControlPoints();
		int iIndexCount = pMesh->GetPolygonVertexCount();
		int* pIndexs = pMesh->GetPolygonVertices();

		mesh.vecVertex.resize(iVertexCount);

		if (iVertexCount <= 0 || iIndexCount <= 0)
		{
			return false;
		}

		for (int i = 0; i < iVertexCount; ++i)
		{
			mesh.vecVertex[i].pos.x = static_cast<float>(pVertexs[i][0]);
			mesh.vecVertex[i].pos.y = static_cast<float>(pVertexs[i][2]);
			mesh.vecVertex[i].pos.z = static_cast<float>(pVertexs[i][1]);
		}

		int iVertexID = 0;

		int iMaterialCount = pMesh->GetNode()->GetMaterialCount();

		mesh.vecIndex.resize(iMaterialCount);

		fbxsdk::FbxGeometryElementMaterial* pMaterial = pMesh->GetElementMaterial();

		int iPolyCount = pMesh->GetPolygonCount();

		for (int i = 0; i < iPolyCount; ++i)
		{
			int iPolySize = pMesh->GetPolygonSize(i);

			int iIndex[3] = {};

			for (int j = 0; j < iPolySize; ++j)
			{
				int iControlIndex = pMesh->GetPolygonVertex(i, j);

				iIndex[j] = iControlIndex;

				LoadNormals(pMesh, iVertexID, iControlIndex, mesh);

				LoadTangents(pMesh, iVertexID, iControlIndex, mesh);

				LoadUVs(pMesh, iVertexID, iControlIndex, mesh);

				++iVertexID;
			}

			int _iIndex = pMaterial ? pMaterial->GetIndexArray().GetAt(i) : 0;

			if (mesh.vecIndex.size() <= _iIndex)
			{
				mesh.vecIndex.resize(_iIndex + 1);
			}

			mesh.vecIndex[_iIndex].push_back(iIndex[0]);
			mesh.vecIndex[_iIndex].push_back(iIndex[2]);
			mesh.vecIndex[_iIndex].push_back(iIndex[1]);
		}

		// Trust FBX-baked normals when present (LoadNormals accumulated them
		// per polygon-vertex above) — DCC apps bake smoothing groups, custom
		// vertex normals, and hard edges that face-cross-product synthesis
		// can't reproduce. Normalize the per-control-point sum so the
		// soft-shaded average is unit-length. Falls back to SetNormals only
		// when the FBX ships without a normal element (raw point clouds,
		// some procedural exporters). Must run before SetTangent below
		// because the tangent computation reads `vertex.normal`.
		if (pMesh->GetElementNormal())
		{
			for (auto& vertex : mesh.vecVertex)
			{
				Vector3& n = vertex.normal;
				float fLen = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
				if (fLen > 1e-6f)
				{
					n.x /= fLen;
					n.y /= fLen;
					n.z /= fLen;
				}
			}
		}
		else
		{
			std::vector<unsigned int> vecFlatIndex;
			for (const auto& vec : mesh.vecIndex)
			{
				vecFlatIndex.insert(vecFlatIndex.end(), vec.begin(), vec.end());
			}
			MeshUtils::SetNormals(mesh.vecVertex, vecFlatIndex);
		}

		// Fallback: FBX files exported without tangent data leave every
		// vertex.tangent at (0,0,0,0), which makes BumpMapping return 0
		// (gray normal GBuffer). Compute tangents from UV derivatives the
		// same way the OBJ pipeline does (MeshLoader.cpp uses SetTangent).
		// SetTangent normalises per-vertex once at the end, so per-material
		// index lists must be flattened into a single pass.
		if (!pMesh->GetElementTangent())
		{
			std::vector<unsigned int> vecFlatIndex;
			for (const auto& vec : mesh.vecIndex)
			{
				vecFlatIndex.insert(vecFlatIndex.end(), vec.begin(), vec.end());
			}
			MeshUtils::SetTangent(mesh.vecVertex, vecFlatIndex);
		}

		LoadAnimation(pMesh, group);

		LoadMaterial(pMesh, mesh);

		LoadTexture(pMesh, mesh);

		return true;
	}

	void FbxLoader::LoadNormals(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh)
	{
		fbxsdk::FbxGeometryElementNormal* pNormal = pNode->GetElementNormal();

		// Mirror LoadTangents/LoadBinormals: skip silently if the mesh
		// shipped without a normal element. Caller's post-loop fallback
		// (MeshUtils::SetNormals) synthesises normals from geometry.
		if (!pNormal)
		{
			return;
		}

		int iNormalIndex = iVertexIndex;

		switch (pNormal->GetMappingMode())
		{
		case fbxsdk::FbxLayerElement::eByPolygonVertex:
		{
			switch (pNormal->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				// Default — iNormalIndex stays at iVertexIndex.
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iNormalIndex = pNormal->GetIndexArray().GetAt(iVertexIndex);
				break;
			}
		}
		break;
		case fbxsdk::FbxLayerElement::eByControlPoint:
		{
			switch (pNormal->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				iNormalIndex = iControlIndex;
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iNormalIndex = pNormal->GetIndexArray().GetAt(iControlIndex);
				break;
			}
		}
		break;
		}

		const fbxsdk::FbxVector4& tNormal = pNormal->GetDirectArray().GetAt(iNormalIndex);

		// Accumulate, don't overwrite. FBX stores normals per polygon-vertex
		// for most meshes (Mixamo, Maya, Max default), so the same control
		// point gets visited once per adjacent triangle. Writing `=` made
		// the last triangle's normal win, producing splotchy shading on
		// shared vertices. Accumulating + post-loop normalize (in LoadFbxMesh
		// after every polygon-vertex is visited) gives the smooth average.
		// Hard edges (rare in skinned meshes) get smoothed away, which is
		// acceptable for the target asset pipeline.
		mesh.vecVertex[iControlIndex].normal.x += static_cast<float>(tNormal[0]);
		mesh.vecVertex[iControlIndex].normal.y += static_cast<float>(tNormal[2]);
		mesh.vecVertex[iControlIndex].normal.z += static_cast<float>(tNormal[1]);
	}

	void FbxLoader::LoadTangents(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh)
	{
		fbxsdk::FbxGeometryElementTangent* pTangent = pNode->GetElementTangent();

		if (!pTangent)
		{
			return;
		}

		int iIndex = iVertexIndex;

		switch (pTangent->GetMappingMode())
		{
		case fbxsdk::FbxLayerElement::eByControlPoint:
		{
			switch (pTangent->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				iIndex = iControlIndex;
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iIndex = pTangent->GetIndexArray().GetAt(iControlIndex);
				break;
			}
		}
		break;
		case fbxsdk::FbxLayerElement::eByPolygonVertex:
		{
			switch (pTangent->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				iIndex = iVertexIndex;
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iIndex = pTangent->GetIndexArray().GetAt(iVertexIndex);
				break;
			}
		}
		break;
		}

		fbxsdk::FbxVector4 vTangent = pTangent->GetDirectArray().GetAt(iIndex);

		mesh.vecVertex[iControlIndex].tangent.x = static_cast<float>(vTangent[0]);
		mesh.vecVertex[iControlIndex].tangent.y = static_cast<float>(vTangent[2]);
		mesh.vecVertex[iControlIndex].tangent.z = static_cast<float>(vTangent[1]);
		// Negate the handedness flag. The Y↔Z position swap above is a
		// reflection (det = -1), which flips the sign of `cross(N, T)` in
		// engine space. shared.hlsl's BumpMapping computes
		//   bitangent = cross(N, T) * tangent.w
		// so to keep `bitangent` pointing the same way relative to the UV
		// derivative basis the FBX baked it for, w must invert. Without this
		// every FBX-baked tangent path renders normal maps inverted on one
		// axis (concave/convex flipped) and breaks mirrored UV consistency.
		mesh.vecVertex[iControlIndex].tangent.w = -static_cast<float>(vTangent[3]);
	}

	void FbxLoader::LoadUVs(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh)
	{
		fbxsdk::FbxLayerElementUV* pUV = pNode->GetElementUV();

		if (!pUV)
		{
			return;
		}

		int iUVIndex = iVertexIndex;

		switch (pUV->GetMappingMode())
		{
		case fbxsdk::FbxLayerElement::eByPolygonVertex:
		{
			switch (pUV->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iUVIndex = pUV->GetIndexArray().GetAt(iVertexIndex);
				break;
			}
		}
		break;
		case fbxsdk::FbxLayerElement::eByControlPoint:
		{
			switch (pUV->GetReferenceMode())
			{
			case fbxsdk::FbxLayerElement::eDirect:
				iUVIndex = iControlIndex;
				break;
			case fbxsdk::FbxLayerElement::eIndexToDirect:
				iUVIndex = pUV->GetIndexArray().GetAt(iControlIndex);
				break;
			}
		}
		break;
		}

		const fbxsdk::FbxVector2& vUV = pUV->GetDirectArray().GetAt(iUVIndex);

		mesh.vecVertex[iControlIndex].uv.x = static_cast<float>(vUV[0] - (int)vUV[0]);
		mesh.vecVertex[iControlIndex].uv.y = 1.f - static_cast<float>(vUV[1] - (int)vUV[1]);
	}

	void FbxLoader::LoadLodGroup(fbxsdk::FbxNode* pNode)
	{
	}

	void FbxLoader::LoadTexture(fbxsdk::FbxGeometry* pGeometry, MESH& mesh)
	{
		fbxsdk::FbxNode* pNode = pGeometry->GetNode();

		if (!pNode)
		{
			return;
		}

		int iMaterialCount = pNode->GetSrcObjectCount();

		for (int i = 0; i < iMaterialCount; ++i)
		{
			fbxsdk::FbxObject* pMaterial = pNode->GetSrcObject(i);

			if (!pMaterial)
			{
				continue;
			}

			for (int j = 0; j < fbxsdk::FbxLayerElement::sTypeTextureCount; ++j)
			{
				fbxsdk::FbxProperty tProperty = pMaterial->FindProperty(fbxsdk::FbxLayerElement::sTextureChannelNames[j]);

				if (!tProperty.IsValid())
				{
					continue;
				}

				int iTextureCount = tProperty.GetSrcObjectCount<fbxsdk::FbxTexture>();

				for (int k = 0; k < iTextureCount; ++k)
				{
					fbxsdk::FbxLayeredTexture* pLayeredTexture = tProperty.GetSrcObject<fbxsdk::FbxLayeredTexture>(k);

					if (!pLayeredTexture)
					{
						fbxsdk::FbxTexture* pTexture = tProperty.GetSrcObject<fbxsdk::FbxTexture>(k);

						if (!pTexture)
						{
							continue;
						}

						FbxFileTexture* pFileTexture = fbxsdk::FbxCast<FbxFileTexture>(pTexture);

						if (!pFileTexture)
						{
							continue;
						}

						{
							char buf[1024];
							sprintf_s(buf,
								"[FbxTexture] channel='%s' name='%s'\n"
								"             FileName        ='%s'\n"
								"             RelativeFileName='%s'\n",
								fbxsdk::FbxLayerElement::sTextureChannelNames[j],
								pTexture->GetName(),
								pFileTexture->GetFileName() ? pFileTexture->GetFileName() : "(null)",
								pFileTexture->GetRelativeFileName() ? pFileTexture->GetRelativeFileName() : "(null)");
							::OutputDebugStringA(buf);
						}

						mesh.m_vecTextureInfo.push_back(TEXTUREINFO{ static_cast<fbxsdk::FbxLayerElement::EType>(j + fbxsdk::FbxLayerElement::sTypeTextureStartIndex), pTexture->GetName() , pFileTexture->GetFileName() });
					}
					else
					{
						assert(false);
					}
				}
			}
		}
	}

	void FbxLoader::LoadMaterialMapping(fbxsdk::FbxMesh* pGeometry)
	{
	}

	void FbxLoader::LoadMaterial(fbxsdk::FbxGeometry* pGeometry, MESH& mesh)
	{
		fbxsdk::FbxNode* pNode = pGeometry->GetNode();

		if (!pNode)
		{
			return;
		}

		if (mesh.name != (pNode->GetName()[0] != '\0' ? pNode->GetName() : pNode->GetMesh()->GetName()))
		{
			return;
		}

		int iMaterialCount = pNode->GetMaterialCount();

		for (int i = 0; i < iMaterialCount; ++i)
		{
			fbxsdk::FbxSurfaceMaterial* pMaterial = pNode->GetMaterial(i);

			if (!pMaterial)
			{
				continue;
			}

			if (pMaterial->GetClassId().Is(fbxsdk::FbxSurfacePhong::ClassId))
			{
				fbxsdk::FbxSurfacePhong* pPhongMaterial = static_cast<fbxsdk::FbxSurfacePhong*>(pMaterial);

				MATERIALINFO material = {};

				material.tMaterial.diffuseColor.w = 1.f;
				material.tMaterial.specularColor.w = 1.f;
				material.tMaterial.emissiveColor.w = 1.f;

				const fbxsdk::FbxDouble3& diffuse = pPhongMaterial->Diffuse.Get();

				material.tMaterial.diffuseColor.x = (float)diffuse[0];
				material.tMaterial.diffuseColor.y = (float)diffuse[1];
				material.tMaterial.diffuseColor.z = (float)diffuse[2];

				const fbxsdk::FbxDouble3& ambient = pPhongMaterial->Ambient.Get();

				material.tMaterial.ambientColor.x = (float)ambient[0];
				material.tMaterial.ambientColor.y = (float)ambient[1];
				material.tMaterial.ambientColor.z = (float)ambient[2];

				const fbxsdk::FbxDouble3& specular = pPhongMaterial->Specular.Get();

				material.tMaterial.specularColor.x = (float)specular[0];
				material.tMaterial.specularColor.y = (float)specular[1];
				material.tMaterial.specularColor.z = (float)specular[2];

				const fbxsdk::FbxDouble3& emissive = pPhongMaterial->Emissive.Get();

				material.tMaterial.emissiveColor.x = (float)emissive[0];
				material.tMaterial.emissiveColor.y = (float)emissive[1];
				material.tMaterial.emissiveColor.z = (float)emissive[2];

				material.tMaterial.fSpecPower = (float)pPhongMaterial->Shininess;

				material.tMaterial.fFraction = (float)pPhongMaterial->ReflectionFactor;

				material.name = pMaterial->GetName();

				mesh.m_vecMaterial.push_back(material);
			}
			else if (pMaterial->GetClassId().Is(fbxsdk::FbxSurfaceLambert::ClassId))
			{
				fbxsdk::FbxSurfaceLambert* pLambertMaterial = static_cast<fbxsdk::FbxSurfaceLambert*>(pMaterial);

				MATERIALINFO material = {};

				material.tMaterial.diffuseColor.w = 1.f;
				material.tMaterial.specularColor.w = 1.f;
				material.tMaterial.emissiveColor.w = 1.f;

				const fbxsdk::FbxDouble3& diffuse = pLambertMaterial->Diffuse.Get();

				material.tMaterial.diffuseColor.x = (float)diffuse[0];
				material.tMaterial.diffuseColor.y = (float)diffuse[1];
				material.tMaterial.diffuseColor.z = (float)diffuse[2];

				const fbxsdk::FbxDouble3& ambient = pLambertMaterial->Ambient.Get();

				material.tMaterial.ambientColor.x = (float)ambient[0];
				material.tMaterial.ambientColor.y = (float)ambient[1];
				material.tMaterial.ambientColor.z = (float)ambient[2];

				const fbxsdk::FbxDouble3& emissive = pLambertMaterial->Emissive.Get();

				material.tMaterial.emissiveColor.x = (float)emissive[0];
				material.tMaterial.emissiveColor.y = (float)emissive[1];
				material.tMaterial.emissiveColor.z = (float)emissive[2];

				material.tMaterial.fSpecPower = 1.f;

				material.name = pMaterial->GetName();

				mesh.m_vecMaterial.push_back(material);
			}
			else
			{
				assert(false);
			}
		}
	}

	void FbxLoader::LoadWeightAndBoneIndex(fbxsdk::FbxCluster* pCluster, int iBoneIndex, MESH& mesh)
	{
		int iIndexCount = pCluster->GetControlPointIndicesCount();

		for (int i = 0; i < iIndexCount; ++i)
		{
			WEIGHT tWeight;

			tWeight.fWeight = static_cast<float>(pCluster->GetControlPointWeights()[i]);
			tWeight.iBoneIndex = iBoneIndex;

			int iClusterIndex = pCluster->GetControlPointIndices()[i];

			mesh.mapWeight[iClusterIndex].push_back(tWeight);
		}
	}

	void FbxLoader::LoadAnimation(fbxsdk::FbxMesh* pMesh, LODGROUP& group)
	{
		int iDeformerCount = pMesh->GetDeformerCount();

		fbxsdk::FbxAMatrix matTransform = GetTransform(pMesh->GetNode());

		for (int i = 0; i < iDeformerCount; ++i)
		{
			fbxsdk::FbxSkin* pDeformer = (fbxsdk::FbxSkin*)pMesh->GetDeformer(i, fbxsdk::FbxDeformer::EDeformerType::eSkin);

			switch (pDeformer->GetSkinningType())
			{
			case fbxsdk::FbxSkin::eRigid:
			case fbxsdk::FbxSkin::eLinear:
			case fbxsdk::FbxSkin::eBlend:
				break;
			default:
				continue;
			}

			int iClusterCount = pDeformer->GetClusterCount();

			for (int j = 0; j < iClusterCount; ++j)
			{
				fbxsdk::FbxCluster* pCluster = pDeformer->GetCluster(j);

				int iBoneIndex = FindBoneIndex(pCluster->GetLink()->GetName());

				LoadWeightAndBoneIndex(pCluster, iBoneIndex, group.tMesh);

				LoadOffsetMatrix(pCluster, matTransform, iBoneIndex, group);

				for (size_t k = 0; k < group.vecSequence.size(); ++k)
				{
					group.vecSequence[k].vecBoneKeyFrame.resize(m_tSkeleton.vecBone.size());
				}

				LoadKeyFrameMatrix(pMesh->GetNode(), pCluster, matTransform, iBoneIndex, group);
			}
		}

		std::unordered_map<int, std::vector<WEIGHT>>::iterator iter = group.tMesh.mapWeight.begin();
		std::unordered_map<int, std::vector<WEIGHT>>::iterator iterEnd = group.tMesh.mapWeight.end();

		for (; iter != iterEnd; ++iter)
		{
			if (iter->second.size() > 4)
			{
				std::qsort(&iter->second[0], iter->second.size(), sizeof(WEIGHT), [](void const* pSrc, void const* pDest)
					{
						if (static_cast<const WEIGHT*>(pSrc)->fWeight < static_cast<const WEIGHT*>(pDest)->fWeight)
						{
							return 1;
						}

						else if (static_cast<const WEIGHT*>(pSrc)->fWeight > static_cast<const WEIGHT*>(pDest)->fWeight)
						{
							return -1;
						}

						return 0;
					});

				iter->second.resize(4);
			}

			float fWeightSum = 0.f;

			for (int i = 0; i < iter->second.size(); ++i)
			{
				fWeightSum += iter->second[i].fWeight;
			}

			for (int i = 0; i < iter->second.size(); ++i)
			{
				if (i < 3)
				{
					group.tMesh.vecVertex[iter->first].blendWeight[i] = iter->second[i].fWeight / fWeightSum;
				}

				group.tMesh.vecVertex[iter->first].blendIndecies[i] = static_cast<float>(iter->second[i].iBoneIndex);
			}
		}
	}

	void FbxLoader::LoadOffsetMatrix(fbxsdk::FbxCluster* pCluster, fbxsdk::FbxAMatrix& _matTrasform, int iBoneIndex, LODGROUP& group)
	{
		fbxsdk::FbxAMatrix matConvert;

		matConvert.SetRow(0, fbxsdk::FbxVector4(1.f, 0.f, 0.f, 0.f));
		matConvert.SetRow(1, fbxsdk::FbxVector4(0.f, 0.f, 1.f, 0.f));
		matConvert.SetRow(2, fbxsdk::FbxVector4(0.f, 1.f, 0.f, 0.f));
		matConvert.SetRow(3, fbxsdk::FbxVector4(0.f, 0.f, 0.f, 1.f));

		fbxsdk::FbxAMatrix matTransform;
		fbxsdk::FbxAMatrix matTransformLink;

		pCluster->GetTransformMatrix(matTransform);
		pCluster->GetTransformLinkMatrix(matTransformLink);

		const fbxsdk::FbxAMatrix& matOffset = matConvert * matTransformLink.Inverse() * matTransform * matConvert.Inverse();

		Matrix _matBone;
		Matrix _matOffset;

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				_matOffset[i][j] = static_cast<float>(matOffset.Get(i, j));
				_matBone[i][j] = static_cast<float>(_matTrasform.Get(i, j));
			}
		}

		m_tSkeleton.vecBone[iBoneIndex].matInvBindPose = _matOffset;
		m_tSkeleton.vecBone[iBoneIndex].matBone = _matBone;
	}

	void FbxLoader::LoadKeyFrameMatrix(fbxsdk::FbxNode* pNode, fbxsdk::FbxCluster* pCluster, fbxsdk::FbxAMatrix& matTrasform, int iBoneIndex, LODGROUP& group)
	{
		fbxsdk::FbxAMatrix matConvert;

		matConvert.SetRow(0, fbxsdk::FbxVector4(1.f, 0.f, 0.f, 0.f));
		matConvert.SetRow(1, fbxsdk::FbxVector4(0.f, 0.f, 1.f, 0.f));
		matConvert.SetRow(2, fbxsdk::FbxVector4(0.f, 1.f, 0.f, 0.f));
		matConvert.SetRow(3, fbxsdk::FbxVector4(0.f, 0.f, 0.f, 1.f));

		for (size_t i = 0; i < group.vecSequence.size(); ++i)
		{
			group.vecSequence[i].vecBoneKeyFrame[iBoneIndex].iBoneIndex = iBoneIndex;

			fbxsdk::FbxLongLong iStart = group.vecSequence[i].tStart.GetFrameCount(group.vecSequence[i].eTimeMode);
			fbxsdk::FbxLongLong iEnd = group.vecSequence[i].tEnd.GetFrameCount(group.vecSequence[i].eTimeMode);

			for (fbxsdk::FbxLongLong j = iStart; j <= iEnd; ++j)
			{
				FbxTime tTime = {};

				tTime.SetFrame(j, group.vecSequence[i].eTimeMode);

				FBXKEYFRAME tKeyFrame;

				fbxsdk::FbxAMatrix tNodeMatrix;
				
				GetGlobalMatrix(pNode, j - iStart, tTime, tNodeMatrix);

				fbxsdk::FbxAMatrix matOffset = matConvert * tNodeMatrix * matConvert.Inverse();

				tKeyFrame.dTime = tTime.GetSecondDouble();

				GetGlobalMatrix(pCluster->GetLink(), j - iStart, tTime, tNodeMatrix);

				tKeyFrame.matTransform = matConvert * matOffset.Inverse() * tNodeMatrix * matConvert.Inverse();

				group.vecSequence[i].vecBoneKeyFrame[iBoneIndex].vecKeyFrame.push_back(tKeyFrame);
			}
		}
	}

	bool FbxLoader::GetGlobalMatrix(fbxsdk::FbxNode* pNode, __int64 iTime, const fbxsdk::FbxTime& tTime, fbxsdk::FbxAMatrix& tNodeMatrix)
	{
		std::unordered_map<fbxsdk::FbxNode*, std::vector<fbxsdk::FbxAMatrix>>::iterator iter = m_mapGlobalMatrix.find(pNode);

		if (iter == m_mapGlobalMatrix.end())
		{
			m_mapGlobalMatrix.insert(std::make_pair(pNode, std::vector<fbxsdk::FbxAMatrix>()));

			iter = m_mapGlobalMatrix.find(pNode);
		}

		if (iter != m_mapGlobalMatrix.end())
		{
			if (static_cast<__int64>(iter->second.size()) <= iTime)
			{
				std::unordered_map<fbxsdk::FbxNode*, std::vector<fbxsdk::FbxAMatrix>>::iterator iterP = m_mapGlobalMatrix.find(pNode->GetParent());

				if (iterP == m_mapGlobalMatrix.end() || static_cast<__int64>(iterP->second.size()) <= iTime)
				{
					tNodeMatrix = pNode->EvaluateGlobalTransform(tTime);
				}
				else
				{
					tNodeMatrix = iterP->second[iTime] * pNode->EvaluateLocalTransform(tTime);
				}

				iter->second.push_back(tNodeMatrix);
			}
			else
			{
				tNodeMatrix = iter->second[iTime];
			}
		}

		return true;
	}

	int FbxLoader::LoadBone(fbxsdk::FbxNode* pNode)
	{
		fbxsdk::FbxNodeAttribute* pAttribute = pNode->GetNodeAttribute();

		if (pAttribute && pAttribute->GetAttributeType() == fbxsdk::FbxNodeAttribute::EType::eSkeleton)
		{
			BONE bone;

			bone.strName = pNode->GetName();

			int iIndex = static_cast<int>(m_tSkeleton.vecBone.size());

			m_tSkeleton.vecBone.push_back(bone);

			for (int i = 0; i < pNode->GetChildCount(); ++i)
			{
				int iChildIndex = LoadBone(pNode->GetChild(i));

				if (iChildIndex != -1)
				{
					m_tSkeleton.vecBone[iChildIndex].iParent = iIndex;
				}
			}

			return iIndex;
		}


		for (int i = 0; i < pNode->GetChildCount(); ++i)
		{
			LoadBone(pNode->GetChild(i));
		}

		return -1;
	}

	void FbxLoader::LoadAnimationClip(fbxsdk::FbxArray<fbxsdk::FbxString*>& vecName)
	{
		fbxsdk::FbxTime::EMode eTimeMode = m_pScene->GetGlobalSettings().GetTimeMode();

		for (int i = 0; i < vecName.GetCount(); ++i)
		{
			fbxsdk::FbxAnimStack* pAnimStack = m_pScene->FindMember<fbxsdk::FbxAnimStack>(vecName[i]->Buffer());

			if (!pAnimStack)
			{
				continue;
			}

			SEQUENCE tSequence;

			tSequence.strTag = pAnimStack->GetName();

			fbxsdk::FbxTakeInfo* pTakeInfo = m_pScene->GetTakeInfo(tSequence.strTag.c_str());

			tSequence.tStart = pTakeInfo->mReferenceTimeSpan.GetStart();
			tSequence.tEnd = pTakeInfo->mReferenceTimeSpan.GetStop();

			tSequence.lFrameLength = tSequence.tEnd.GetFrameCount(eTimeMode) - tSequence.tStart.GetFrameCount(eTimeMode);
			tSequence.eTimeMode = eTimeMode;

			m_vecSequence.push_back(tSequence);
		}
	}

	float FbxLoader::GetCurve(int& iIndex, const char* pText, fbxsdk::FbxNode* pNode, fbxsdk::FbxAnimLayer* pAnimLayer) const
	{
		fbxsdk::FbxAnimCurve* pAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, pText);

		int iKeyCount = pAnimCurve->KeyGetCount();

		float fKeyValue = 0.f;
		FbxTime tKeyTime;

		for (int i = 0; i < iKeyCount; ++i)
		{
			fKeyValue = static_cast<float>(pAnimCurve->KeyGetValue(i));
			tKeyTime = pAnimCurve->KeyGetTime(i);

			iIndex = static_cast<int>(tKeyTime.GetSecondDouble());
		}

		return fKeyValue;
	}

	int FbxLoader::FindBoneIndex(const std::string& strBone) const
	{
		for (int i = 0; i < static_cast<int>(m_tSkeleton.vecBone.size()); ++i)
		{
			if (m_tSkeleton.vecBone[i].strName == strBone)
			{
				return i;
			}
		}

		return -1;
	}

	void FbxLoader::LoadAnimation()
	{
		int iAnimStackCount = m_pScene->GetSrcObjectCount<fbxsdk::FbxAnimStack>();

		for (int i = 0; i < iAnimStackCount; ++i)
		{
			fbxsdk::FbxAnimStack* pAnimStack = m_pScene->GetSrcObject<fbxsdk::FbxAnimStack>(i);

			int iAnimLayer = pAnimStack->GetMemberCount<FbxAnimLayer>();

			for (int j = 0; j < iAnimLayer; ++j)
			{
				FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(j);

				LoadAnimation(m_pScene->GetRootNode(), pAnimLayer, i);
			}
		}
	}

	void FbxLoader::LoadAnimation(fbxsdk::FbxNode* pNode, FbxAnimLayer* pAnimLayer, int iAnimStackIndex)
	{
		int iBone = FindBoneIndex(pNode->GetName());

		if (iBone != -1)
		{
			std::vector<Vector3> vecPos(m_vecSequence[iAnimStackIndex].lFrameLength + 1);
			std::vector<Vector3> vecRot(m_vecSequence[iAnimStackIndex].lFrameLength + 1);
			std::vector<Vector3> vecScale(m_vecSequence[iAnimStackIndex].lFrameLength + 1);

			for (int i = 0; i < static_cast<int>(vecScale.size()); ++i)
			{
				vecScale[i] = 1.f;
			}

			m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame.resize(m_vecSequence[iAnimStackIndex].lFrameLength+1 );

			LoadAnimationPosition(pNode->LclTranslation, pAnimLayer, iAnimStackIndex, iBone, vecPos, 0.f);

			LoadAnimationPosition(pNode->LclRotation, pAnimLayer, iAnimStackIndex, iBone, vecRot, 0.f);

			LoadAnimationPosition(pNode->LclScaling, pAnimLayer, iAnimStackIndex, iBone, vecScale, 1.f);

			int iParentIndex = -1;

			fbxsdk::FbxNode* pParentNode = pNode->GetParent();

			if (pParentNode)
			{
				iParentIndex = FindBoneIndex(pParentNode->GetName());
			}

			for (int i = 0; i < m_vecSequence[iAnimStackIndex].lFrameLength + 1; ++i)
			{
				fbxsdk::FbxAMatrix matT;

				matT.SetT({ vecPos[i].x, vecPos[i].y, vecPos[i].z ,0.f });

				fbxsdk::FbxAMatrix matS;

				matS.SetS({ vecScale[i].x, vecScale[i].y, vecScale[i].z,0.f });

				fbxsdk::FbxAMatrix matR;

				matR.SetR({ vecRot[i].x, vecRot[i].y, vecRot[i].z ,0.f });

				fbxsdk::FbxTime tTime;

				tTime.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

				m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].matTransform = 
					matT * matR * matS;
			}
		}

	 	int iChildCount = pNode->GetChildCount();

		for (int i = 0; i < iChildCount; ++i)
		{
			LoadAnimation(pNode->GetChild(i), pAnimLayer, iAnimStackIndex);
		}
	}

	void FbxLoader::LoadAnimationPosition(fbxsdk::FbxPropertyT<fbxsdk::FbxDouble3>& tProp, FbxAnimLayer* pAnimLayer, int iAnimStackIndex, int iBone, std::vector<Vector3>& vecPos, float fDefaultValue)
	{
		int iHour;
		int iMinute;
		int iSecond;
		int iField;
		int iResidual;

		fbxsdk::FbxAnimCurve* pCurvePosX = tProp.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X);

		if (pCurvePosX)
		{
			int iKeyCount = pCurvePosX->KeyGetCount();

			int iPrevFrame = -1;

			float fPrevValue = fDefaultValue;

			for (int k = 0; k < iKeyCount; ++k)
			{
				float fX = pCurvePosX->KeyGetValue(k);

				int iFrame;

				pCurvePosX->KeyGetTime(k).GetTime(iHour, iMinute, iSecond, iFrame, iField, iResidual, m_vecSequence[iAnimStackIndex].eTimeMode);

				int iFramePerSecond = 0;

				switch (m_vecSequence[iAnimStackIndex].eTimeMode)
				{
				case fbxsdk::FbxTime::eFrames24:
					iFramePerSecond = 24;
					break;
				case fbxsdk::FbxTime::eFrames30:
					iFramePerSecond = 30;
					break;
				default:
					assert(false);
					break;
				}

				iFrame += (iSecond + (iMinute + iHour * 60) * 60) * iFramePerSecond;

				if (vecPos.size() <= iFrame)
				{
					vecPos.resize(iFrame + 1);
				}

				if (iFrame - iPrevFrame - 1 == 0)
				{
					fbxsdk::FbxTime time;

					time.SetFrame(iFrame, m_vecSequence[iAnimStackIndex].eTimeMode);

					if (m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame.size() > iFrame)
					{
						m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[iFrame].dTime = time.GetSecondDouble();
					}

					vecPos[iFrame].x = fX;
				}
				else
				{
					for (int i = iPrevFrame + 1; i <= iFrame; ++i)
					{
						fbxsdk::FbxTime time;

						time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

						if (m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame.size() > i)
						{
							m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();
						}

						// Delegate per-frame sampling to the FBX SDK. Evaluate
						// honours each key's own interpolation type and tangents
						// (Constant, Linear, Cubic Hermite, TCB, custom curves),
						// which removes the manual switch and fixes the previous
						// "assert(false)" crash on cubic Mixamo / Maya animations.
						vecPos[i].x = pCurvePosX->Evaluate(time);
					}
				}

				fPrevValue = fX;
				iPrevFrame = iFrame;
			}

			for (int i = iPrevFrame + 1; i < m_vecSequence[iAnimStackIndex].lFrameLength + 1; ++i)
			{
				fbxsdk::FbxTime time;

				time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

				m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();

				vecPos[i].x = fPrevValue;
			}
		}

		fbxsdk::FbxAnimCurve* pCurvePosY = tProp.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);

		if (pCurvePosY)
		{
			int iKeyCount = pCurvePosY->KeyGetCount();

			int iPrevFrame = -1;

			float fPrevValue = fDefaultValue;

			for (int k = 0; k < iKeyCount; ++k)
			{
				float fY = pCurvePosY->KeyGetValue(k);

				int iFrame;

				pCurvePosY->KeyGetTime(k).GetTime(iHour, iMinute, iSecond, iFrame, iField, iResidual, m_vecSequence[iAnimStackIndex].eTimeMode);

				int iFramePerSecond = 0;

				switch (m_vecSequence[iAnimStackIndex].eTimeMode)
				{
				case fbxsdk::FbxTime::eFrames24:
					iFramePerSecond = 24;
					break;
				case fbxsdk::FbxTime::eFrames30:
					iFramePerSecond = 30;
					break;
				default:
					assert(false);
					break;
				}

				iFrame += (iSecond + (iMinute + iHour * 60) * 60) * iFramePerSecond;

				if (vecPos.size() <= iFrame)
				{
					vecPos.resize(iFrame + 1);
				}

				if (iFrame - iPrevFrame - 1 == 0)
				{
					vecPos[iFrame].y = fY;
				}
				else
				{
					for (int i = iPrevFrame + 1; i <= iFrame; ++i)
					{
						fbxsdk::FbxTime time;

						time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

						if (m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame.size() > i)
						{
							m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();
						}

						// SDK evaluation — same rationale as the X channel above.
						vecPos[i].y = pCurvePosY->Evaluate(time);
					}
				}

				fPrevValue = fY;
				iPrevFrame = iFrame;
			}

			for (int i = iPrevFrame + 1; i < m_vecSequence[iAnimStackIndex].lFrameLength + 1; ++i)
			{
				fbxsdk::FbxTime time;

				time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

				m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();

				vecPos[i].y = fPrevValue;
			}
		}

		fbxsdk::FbxAnimCurve* pCurvePosZ = tProp.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

		if (pCurvePosZ)
		{
			int iKeyCount = pCurvePosZ->KeyGetCount();

			int iPrevFrame = -1;

			float fPrevValue = fDefaultValue;

			for (int k = 0; k < iKeyCount; ++k)
			{
				float fZ = pCurvePosZ->KeyGetValue(k);

				int iFrame;

				pCurvePosZ->KeyGetTime(k).GetTime(iHour, iMinute, iSecond, iFrame, iField, iResidual, m_vecSequence[iAnimStackIndex].eTimeMode);

				int iFramePerSecond = 0;

				switch (m_vecSequence[iAnimStackIndex].eTimeMode)
				{
				case fbxsdk::FbxTime::eFrames24:
					iFramePerSecond = 24;
					break;
				case fbxsdk::FbxTime::eFrames30:
					iFramePerSecond = 30;
					break;
				default:
					assert(false);
					break;
				}

				iFrame += (iSecond + (iMinute + iHour * 60) * 60) * iFramePerSecond;

				if (vecPos.size() <= iFrame)
				{
					vecPos.resize(iFrame + 1);
				}

				if (iFrame - iPrevFrame - 1 == 0)
				{
					vecPos[iFrame].z = fZ;
				}
				else
				{
					for (int i = iPrevFrame + 1; i <= iFrame; ++i)
					{
						fbxsdk::FbxTime time;

						time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

						if (m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame.size() > i)
						{
							m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();
						}

						// SDK evaluation — same rationale as the X channel above.
						vecPos[i].z = pCurvePosZ->Evaluate(time);
					}
				}

				fPrevValue = fZ;
				iPrevFrame = iFrame;
			}

			for (int i = iPrevFrame + 1; i < m_vecSequence[iAnimStackIndex].lFrameLength + 1; ++i)
			{
				fbxsdk::FbxTime time;

				time.SetFrame(i, m_vecSequence[iAnimStackIndex].eTimeMode);

				m_vecSequence[iAnimStackIndex].vecBoneKeyFrame[iBone].vecKeyFrame[i].dTime = time.GetSecondDouble();

				vecPos[i].z = fPrevValue;
			}
		}
	}

	void FbxLoader::LoadBindPose()
	{
		int iCount = m_pScene->GetPoseCount();

		for (int i = 0; i < iCount && i < 1; ++i)
		{
			fbxsdk::FbxPose* pPose = m_pScene->GetPose(i);

			if (!pPose->IsBindPose())
			{
				continue;
			}

			int iJointCount = pPose->GetCount();

			m_vecBindPose.resize(m_tSkeleton.vecBone.size());

			fbxsdk::FbxMatrix mat;

			mat.SetRow(0, { 1.f, 0.f, 0.f, 0.f });
			mat.SetRow(1, { 0.f, 0.f, 1.f, 0.f });
			mat.SetRow(2, { 0.f, 1.f, 0.f, 0.f });
			mat.SetRow(3, { 0.f, 0.f, 0.f, 1.f});

			for (int j = 0; j < iJointCount; ++j)
			{
				int iBoneIndex = FindBoneIndex(pPose->GetNodeName(j).GetCurrentName());

				if (iBoneIndex >= 0 && m_vecBindPose.size() > iBoneIndex)
				{
					m_vecBindPose[iBoneIndex] = pPose->GetMatrix(j);
				}
			}
		}
	}

	bool FbxLoader::LoadOBJ(const TCHAR* pFileName, const std::string& strPathKey)
	{
		TCHAR strFullPath[MAX_PATH] = {};

		const TCHAR* pPath = CPathManager::GetInst()->FindPath(strPathKey);

		if (pPath)
		{
			wcscpy_s(strFullPath, pPath);
		}

		wcscat_s(strFullPath, pFileName);

		char strFull[MAX_PATH] = {};

#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, strFullPath, -1, strFull, MAX_PATH, 0, 0);
#else
		strcpy_s(strFull, strFullPath);
#endif

		FILE* pFile = nullptr;

		fopen_s(&pFile, strFull, "rt");

		if (pFile)
		{
			LODGROUP group;

			m_tScene.vecLODGroup.push_back(group);

			PLODGROUP pGroup = &m_tScene.vecLODGroup.back();

			bool bSame = true;

			std::vector<Vector3> vecPos;
			std::vector<unsigned int> vecSubIndex;

			std::vector<DirectX::XMFLOAT2> vecUV;
			std::vector<Vector3> vecNormal;

			bool bHasNormal = false;
			bool bHasUV = false;
			int iNormalCount = 0;

			int iPrevVertex = 0;

			int iPrevPos = 0;

			int iPrevUV = 0;

			int iPrevNormal = 0;

			bool bPath = true;

			while (bPath)
			{
				char strLine[MAX_PATH] = {};

				char* pResult = fgets(strLine, MAX_PATH, pFile);


				if (!pResult || !strcmp(pResult, ""))
				{
					break;
				}

				switch (pResult[0])
				{
				case '#':
					break;
				case 'n':
				case 'o':
				case 'g':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (pContext)
					{
						pContext[strlen(pContext) - 1] = 0;

						if (vecSubIndex.size())
						{
							iPrevPos = static_cast<int>(vecPos.size());
							iPrevUV = static_cast<int>(vecUV.size());
							iPrevNormal = static_cast<int>(vecNormal.size());
							iPrevVertex = static_cast<int>(pGroup->tMesh.vecVertex.size());

							pGroup->tMesh.vecIndex.push_back(vecSubIndex);

							vecSubIndex.clear();
						}
					}
				}
				break;
				case 'v':

					switch (pResult[1])
					{
					case 't':
					{
						bHasUV = true;

						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						_pResult = strtok_s(nullptr, " ", &pContext);

						float fU = (float)atof(_pResult);

						_pResult = strtok_s(nullptr, " ", &pContext);

						float fV = (float)atof(_pResult);

						vecUV.push_back({ fU, fV });
					}
					break;
					case 'n':
					{
						bHasNormal = true;

						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						Vector3 vNormal;

						for (int i = 0; i < 3; ++i)
						{
							_pResult = strtok_s(nullptr, " ", &pContext);

							vNormal[i] = (float)atof(_pResult);
						}

						vecNormal.push_back(vNormal);

						++iNormalCount;
					}
					break;
					default:
					{
						char* pContext = nullptr;
						char* _pResult = strtok_s(pResult, " ", &pContext);

						Vector3 position;

						for (int i = 0; i < 3; ++i)
						{
							_pResult = strtok_s(nullptr, " ", &pContext);

							position[i] = (float)atof(_pResult);
						}

						vecPos.push_back(position);
					}
					break;
					}
					break;
				case 'f':
				{
					char* pContext = nullptr;
					char* _pResult = strtok_s(pResult, " ", &pContext);

					std::vector<int> vecVertexSub;

					while (true)
					{
						_pResult = strtok_s(nullptr, " ", &pContext);

						if (!_pResult)
						{
							break;
						}

						char* _pContext = nullptr;

						char* __pResult = strtok_s(_pResult, "/", &_pContext);

						char* __pResult2 = strtok_s(nullptr, "/", &_pContext);

						unsigned int iVertex = atoi(__pResult);

						if (!iVertex)
						{
							break;
						}

						unsigned int iIndex = 0;

						if (__pResult2)
						{
							iIndex = atoi(__pResult2);
						}

						unsigned int iNormalIndex = 0;

						if (_pContext)
						{
							iNormalIndex = atoi(_pContext);
						}

						if (bSame &&
							vecPos.size() - iPrevPos == vecUV.size() - iPrevUV &&
							vecNormal.size() - iPrevNormal == vecUV.size() - iPrevUV)
						{
							if (pGroup->tMesh.vecVertex.size() < iVertex)
							{
								pGroup->tMesh.vecVertex.resize(iVertex);
							}

							pGroup->tMesh.vecVertex[iVertex - 1].pos = vecPos[iVertex - 1];

							if (bHasUV && iIndex)
							{
								pGroup->tMesh.vecVertex[iVertex - 1].uv.x = vecUV[iIndex - 1].x;
								pGroup->tMesh.vecVertex[iVertex - 1].uv.y = 1.f - vecUV[iIndex - 1].y;
							}

							if (bHasNormal && iNormalIndex)
							{
								pGroup->tMesh.vecVertex[iVertex - 1].normal = vecNormal[iNormalIndex - 1];
							}

							vecVertexSub.push_back(iVertex - 1);
						}
						else
						{
							bSame = false;

							VertexStandard vVertex;

							vVertex.pos = vecPos[iVertex - 1];

							if (bHasUV)
							{
								vVertex.uv.x = vecUV[iIndex - 1].x;
								vVertex.uv.y = 1.f - vecUV[iIndex - 1].y;
							}

							if (bHasNormal)
							{
								vVertex.normal = vecNormal[iNormalIndex - 1];
							}

							vecVertexSub.push_back(static_cast<int>(pGroup->tMesh.vecVertex.size()));

							pGroup->tMesh.vecVertex.push_back(vVertex);
						}
					}

					switch (vecVertexSub.size())
					{
					case 3:
						for (size_t i = 0; i < vecVertexSub.size(); ++i)
						{
							vecSubIndex.push_back(vecVertexSub[i]);
						}

						break;
					case 4:
						vecSubIndex.push_back(vecVertexSub[0]);
						vecSubIndex.push_back(vecVertexSub[1]);
						vecSubIndex.push_back(vecVertexSub[2]);

						vecSubIndex.push_back(vecVertexSub[0]);
						vecSubIndex.push_back(vecVertexSub[2]);
						vecSubIndex.push_back(vecVertexSub[3]);
						break;
					default:
						for (size_t i = 0; i < vecVertexSub.size() - 2; ++i)
						{
							vecSubIndex.push_back(vecVertexSub[0]);
							vecSubIndex.push_back(vecVertexSub[i + 1]);
							vecSubIndex.push_back(vecVertexSub[i + 2]);
						}
						break;
					}

				}
				break;
				case 'm':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (!strcmp(_pResult, "mtllib"))
					{
						int iLength = static_cast<int>(strlen(strFull));

						for (int i = iLength - 1; i >= 0; --i)
						{
							if (strFull[i] == '/' || strFull[i] == '\\')
							{
								memset(strFull + i + 1, 0, iLength - i);
								break;
							}
						}

						strcat_s(strFull, pContext);

						strFull[strlen(strFull) - 1] = 0;
					}
				}
				break;
				case 'u':
				{
					char* pContext = nullptr;

					char* _pResult = strtok_s(pResult, " ", &pContext);

					if (!strcmp(_pResult, "usemtl"))
					{
						pContext[strlen(pContext) - 1] = 0;
					}
				}
				break;
				default:
					break;
				}
			}

			if (vecSubIndex.size())
			{
				pGroup->tMesh.vecIndex.push_back(vecSubIndex);
			}

			fclose(pFile);

			return true;
		}

		return false;
	}

}



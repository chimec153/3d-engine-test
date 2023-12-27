#pragma once

#include "../Core/Macro.h"
#include "../Types.h"
namespace fbxsdk
{
	class FbxScene;
	class FbxManager;
	class FbxNode;
	class FbxMesh;
	class FbxGeometry;
	class FbxLayerElement;
	class FbxCluster;
}

namespace Engine
{
	class ENGINE_DLL FbxLoader
	{
	public:
		typedef struct _tagTextureInfo
		{
			fbxsdk::FbxLayerElement::EType type;
			std::string name;
			std::string strFullPath;
		}TEXTUREINFO, * PTEXTUREINFO;

		typedef struct _tagMaterialInfo
		{
			MATERIAL tMaterial;
			std::string name;
		}MATERIALINFO, * PMATERIALINFO;

		typedef struct _tagWeight
		{
			int iBoneIndex;
			float fWeight;
		}WEIGHT, * PWEIGHT;

		typedef struct _tagMesh
		{
			std::string name;
			int	iLodId;
			float fLodThreshold;
			std::vector<std::vector<unsigned int>>	vecIndex;
			std::vector<_tagVertexStandard> vecVertex;
			std::vector<unsigned int> vecMaterialIndex;
			std::vector<unsigned int> vecMaterialIndexUsed;
			std::vector<_tagTextureInfo> m_vecTextureInfo;
			std::vector<MATERIALINFO>	m_vecMaterial;
			std::unordered_map<int, std::vector<WEIGHT>> mapWeight;
		}MESH, * PMESH;

		typedef struct _tagSkeleton
		{
			std::vector<BONE> vecBone;
		}SKELETON, * PSKELETON;

		typedef struct _tagFbxKeyFrame
		{
			fbxsdk::FbxAMatrix matTransform;
			double dTime;
		}FBXKEYFRAME, * PFBXKEYFRAME;

		typedef struct _tagFbxBoneKeyFrame
		{
			int iBoneIndex;
			std::vector<FBXKEYFRAME> vecKeyFrame;
		}FBXBONEKEYFRAME, * PFBXBONEKEYFRAME;


		typedef struct _tagSequence
		{
			std::string strTag;
			fbxsdk::FbxTime tStart;
			fbxsdk::FbxTime tEnd;
			fbxsdk::FbxTime::EMode eTimeMode;
			fbxsdk::FbxLongLong lFrameLength;
			std::vector<FBXBONEKEYFRAME> vecBoneKeyFrame;
		}SEQUENCE, * PSEQUENCE;
		typedef struct _tagLodGroup
		{
			std::string name;
			_tagMesh tMesh;
			std::vector<SEQUENCE> vecSequence;
		}LODGROUP, * PLODGROUP;

		typedef struct _tagScene
		{
			std::vector<_tagLodGroup> vecLODGroup;
		}SCENE, * PSCENE;
	public:
		FbxLoader();
		~FbxLoader();

	private:
		class fbxsdk::FbxScene* m_pScene;
		class fbxsdk::FbxManager* m_pManager;
		float m_fSceneScale;
		SCENE m_tScene;
		bool m_bCalculateNormal;
		bool m_bCalculateTangent;
		std::vector<SEQUENCE> m_vecSequence;
		SKELETON m_tSkeleton;
		std::unordered_map<fbxsdk::FbxNode*, std::vector<fbxsdk::FbxAMatrix>> m_mapGlobalMatrix;
		std::vector<fbxsdk::FbxMatrix> m_vecBindPose;

	public:
		bool Init();
		bool LoadFile(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);
		bool LoadScene(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);
		int GetLODCount()	const;
		std::vector<VertexStandard>& GetVertexData(int iIndex = 0);
		std::string GetMeshName(int iIndex = 0)	const;
		const std::vector<std::vector<unsigned int>>& GetIndexData(int iIndex = 0)	const;
		bool IsCalculatedTangent()	const;
		const std::vector<TEXTUREINFO>& GetTextures(int iIndex = 0)	const;
		const std::vector<MATERIALINFO>& GetMaterials(int iIndex = 0)	const;
		const std::vector<SEQUENCE>& GetSequences(int iIndex)	const;
		const std::vector<SEQUENCE>& GetSequences()	const;
		const SKELETON& GetSkeleton(int iIndex = 0) const;
		void LoadOBJ(const TCHAR* pFileName, const std::string& strPathKey);

	private:
		fbxsdk::FbxAMatrix GetTransform(fbxsdk::FbxNode* pMesh);
		void LoadScene(fbxsdk::FbxNode* pNode = nullptr);
		void LoadMesh(fbxsdk::FbxNode* pNode, LODGROUP& vecMesh);
		bool LoadMeshData(fbxsdk::FbxMesh* pNode, MESH& mesh, LODGROUP& vecMesh);
		void LoadNormals(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh);
		void LoadTangents(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh);
		void LoadBiTangents(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh);
		void LoadUVs(fbxsdk::FbxMesh* pNode, int iVertexIndex, int iControlIndex, MESH& mesh);
		void LoadLodGroup(fbxsdk::FbxNode* pNode);
		void LoadTexture(fbxsdk::FbxGeometry* pGeometry, MESH& mesh);
		void LoadMaterialMapping(fbxsdk::FbxMesh* pGeometry);
		void LoadMaterial(fbxsdk::FbxGeometry* pGeometry, MESH& mesh);
		void LoadWeightAndBoneIndex(fbxsdk::FbxCluster* pNode, int iBoneIndex, MESH& mesh);
		void LoadAnimation(fbxsdk::FbxMesh* pNode, LODGROUP& group);
		void LoadOffsetMatrix(fbxsdk::FbxCluster* pNode, fbxsdk::FbxAMatrix& matTrasform, int iBoneIndex, LODGROUP& group);
		void LoadKeyFrameMatrix(fbxsdk::FbxNode* pNode, fbxsdk::FbxCluster* pCluster, fbxsdk::FbxAMatrix& matTrasform, int iBoneIndex, LODGROUP& group);
		bool GetGlobalMatrix(fbxsdk::FbxNode* pNode, __int64 iTime, const fbxsdk::FbxTime& tTime, fbxsdk::FbxAMatrix& tMatrix);
		int LoadBone(fbxsdk::FbxNode* pNode);
		void LoadAnimationClip(fbxsdk::FbxArray<fbxsdk::FbxString*>& vecName);
		float GetCurve(int& iIndex, const char* pText, fbxsdk::FbxNode* pNode, fbxsdk::FbxAnimLayer* pAnimLayer)	const;
		int FindBoneIndex(const std::string& strBone)	const;
		void LoadAnimation();
		void LoadAnimation(fbxsdk::FbxNode* pNode, FbxAnimLayer* pAnimLayer, int iAnimStackIndex);
		void LoadAnimationPosition(fbxsdk::FbxPropertyT<fbxsdk::FbxDouble3>& tProp, FbxAnimLayer* pAnimLayer, int iAnimStackIndex, int iBone, std::vector<Vector3>& vecPos, float fDefaultValue);
		void LoadBindPose();
	};

}
#include "ResourceManager.h"
#include "../Animation/Skeleton.h"
#include "../Bindable/FbxLoader.h"
#include "../Core/Window.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/Texture.h"
#include "../Animation/Sequence.h"

namespace Engine
{
	ResourceManager* ResourceManager::m_pInst = nullptr;

	ResourceManager::ResourceManager()
	{
	}

	ResourceManager::~ResourceManager()
	{
	}

	std::shared_ptr<Skeleton> ResourceManager::CreateSkeleton(const std::string& strTag, const std::vector<BONE>& vecBone)
	{
		std::shared_ptr<Skeleton> pSkeleton = FindSkeleton(strTag);

		if (pSkeleton)
		{
			return nullptr;
		}

		pSkeleton = std::make_shared<Skeleton>();

		pSkeleton->SetBone(vecBone);

		m_mapSkeleton.insert(std::make_pair(strTag, pSkeleton));

		return pSkeleton;
	}

	std::shared_ptr<Skeleton> ResourceManager::FindSkeleton(const std::string& strTag) const
	{
		std::unordered_map<std::string, std::shared_ptr<Skeleton>>::const_iterator iter = m_mapSkeleton.find(strTag);

		if (iter == m_mapSkeleton.end())
		{
			return nullptr;
		}

		return iter->second;
	}

	std::shared_ptr<Sequence> ResourceManager::CreateSequence(const std::string& strTag)
	{
		std::shared_ptr<Sequence> pSequence = FindSequence(strTag);

		if (pSequence)
		{
			return nullptr;
		}

		pSequence = std::make_shared<Sequence>();

		m_mapSequence.insert(std::make_pair(strTag, pSequence));

		return pSequence;
	}

	std::shared_ptr<Sequence> ResourceManager::FindSequence(const std::string& strTag) const
	{
		std::unordered_map<std::string, std::shared_ptr<Sequence>>::const_iterator iter = m_mapSequence.find(strTag);

		if (iter == m_mapSequence.end())
		{
			return nullptr;
		}

		return iter->second;
	}

	std::shared_ptr<Animation> ResourceManager::FindAnimation(const std::string& strTag) const
	{
		std::unordered_map<std::string, std::shared_ptr<Animation>>::const_iterator iter = m_mapAnimation.find(strTag);

		if (iter == m_mapAnimation.end())
		{
			return nullptr;
		}

		return iter->second;
	}

	void ResourceManager::LoadFile(const TCHAR* pFilePath, const std::string& strPathKey)
	{
		FbxLoader loader;

		loader.Init();

		if (!loader.LoadFile(pFilePath, strPathKey))
		{
			assert(false);
			return;
		}

		int iLodCount = loader.GetLODCount();

		for (int i = 0; i < iLodCount; ++i)
		{
			const std::string& strMesh = loader.GetMeshName(i);

			const std::vector<VertexStandard>& vecVertex = loader.GetVertexData(i);

			const std::vector<std::vector<unsigned int>>& vecIndex = loader.GetIndexData(i);

			StaticCreateBindable<class Mesh>(strMesh, vecVertex, vecIndex);

			const std::vector<FbxLoader::MATERIALINFO>& vecMaterial = loader.GetMaterials(i);

			for (int j = 0; j < vecMaterial.size(); ++j)
			{
				if (StaticFindBindable<Material>(vecMaterial[j].name))
				{
					continue;
				}

				const std::shared_ptr<class Material>& pMaterial = StaticCreateBindable<Material>(vecMaterial[j].name);

				pMaterial->SetMaterial(vecMaterial[j].tMaterial);
			}

			const std::vector<FbxLoader::TEXTUREINFO>& vecTexture = loader.GetTextures(i);

			for (int j = 0; j < vecTexture.size(); ++j)
			{
				if (StaticFindBindable<Texture>(vecTexture[j].name))
				{
					continue;
				}

				switch (vecTexture[j].type)
				{
				case fbxsdk::FbxLayerElement::eTextureDiffuse:
					StaticCreateBindable<Texture>(vecTexture[j].name, vecTexture[j].strFullPath.c_str(), 0);
					break;
				case fbxsdk::FbxLayerElement::eTextureNormalMap:
				case fbxsdk::FbxLayerElement::eTextureBump:
					StaticCreateBindable<Texture>(vecTexture[j].name, vecTexture[j].strFullPath.c_str(), 1);
					break;
				case fbxsdk::FbxLayerElement::eTextureSpecular:
					StaticCreateBindable<Texture>(vecTexture[j].name, vecTexture[j].strFullPath.c_str(), 2);
					break;
				case fbxsdk::FbxLayerElement::eTextureEmissive:
					StaticCreateBindable<Texture>(vecTexture[j].name, vecTexture[j].strFullPath.c_str(), 3);
					break;
				}
			}
		}

		const FbxLoader::SKELETON& tSkeleton = loader.GetSkeleton();

		std::shared_ptr<Skeleton> pSkeleton = nullptr;

		if (tSkeleton.vecBone.size())
		{
			pSkeleton = CreateSkeleton(loader.GetMeshName() + "_Skeleton", tSkeleton.vecBone);
		}

		const std::vector<FbxLoader::SEQUENCE>& vecSequence = loader.GetSequences();

		for (int j = 0; j < vecSequence.size(); ++j)
		{
			std::shared_ptr<Sequence> pSequence = CreateSequence(vecSequence[j].strTag);

			pSequence->SetSequance(vecSequence[j].vecBoneKeyFrame);

			//pSequence->SetSkeleton(pSkeleton);
		}
	}

	void ResourceManager::LoadSkeleton(const char* pFilePath, const std::string& strPathKey)
	{
		std::shared_ptr<Skeleton> pSkeleton = std::make_shared<Skeleton>();

		pSkeleton->LoadFromPath(pFilePath, strPathKey);

		m_mapSkeleton.insert(std::make_pair(pSkeleton->GetTag(), pSkeleton));
	}

	void ResourceManager::LoadSequence(const char* pFilePath, const std::string& strPathKey)
	{
		std::shared_ptr<Sequence> pSequence = std::make_shared<Sequence>();

		pSequence->LoadFromPath(pFilePath, strPathKey);

		m_mapSequence.insert(std::make_pair(pSequence->GetTag(), pSequence));
	}
}
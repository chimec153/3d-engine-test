#include "ResourceManager.h"
#include "../Animation/Skeleton.h"
#include "../Bindable/FbxLoader.h"
#include "../Core/Window.h"
#include "../Bindable/BindableManager.h"
#include "../Bindable/Texture.h"
#include "../Animation/Sequence.h"
#include "../Bindable/Camera.h"
#include "../Bindable/Transform.h"
#include "../Sound/Sound.h"

namespace Engine
{
	ResourceManager* ResourceManager::m_pInst = nullptr;

	ResourceManager::ResourceManager()	:
		m_pSoundSystem(nullptr)
	{
	}

	ResourceManager::~ResourceManager()
	{
		m_mapSound.clear();

		if (m_pSoundSystem)
		{
			m_pSoundSystem->close();

			m_pSoundSystem->release();

			m_pSoundSystem = nullptr;
		}
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
	std::shared_ptr<Sound> ResourceManager::CreateSound(const std::string& strSound, const char* pFilePath, const std::string& strPathKey, float fMin, float fMax, FMOD_MODE b3D, bool bLoop)
	{
		std::shared_ptr<Sound> pSound = FindSound(strSound); 

		if (pSound)
		{
			return nullptr;
		}

		pSound = std::make_shared<Sound>(m_pSoundSystem, strSound, pFilePath, strPathKey, fMin, fMax, b3D, bLoop);

		m_mapSound.insert(std::make_pair(strSound, pSound));

		return pSound;
	}
	std::shared_ptr<Sound> ResourceManager::FindSound(const std::string& strSound) const
	{
		std::unordered_map<std::string, std::shared_ptr<Sound>>::const_iterator iter = m_mapSound.find(strSound);

		if (iter == m_mapSound.end())
		{
			return nullptr;
		}

		return iter->second;
	}
	void ResourceManager::Play_Sound(const std::string& strSound)
	{
		std::shared_ptr<Sound> pSound = FindSound(strSound);

		if (!pSound)
		{
			return;
		}

		pSound->Play();
	}
	void ResourceManager::Stop_Sound(const std::string& strSound)
	{
		std::shared_ptr<Sound> pSound = FindSound(strSound);

		if (!pSound)
		{
			return;
		}

		pSound->Stop();
	}
	bool ResourceManager::Init()
	{
		if (FMOD_OK != FMOD::System_Create(&m_pSoundSystem))
		{
			return false;
		}

		void* pExtraDriverData = nullptr;

		if (FMOD_OK != m_pSoundSystem->init(100, FMOD_INIT_NORMAL, &pExtraDriverData))
		{
			return false;
		}

		if (FMOD_OK != m_pSoundSystem->set3DSettings(1.0, 1.f, 1.f))
		{
			return false;
		}

		return true;
	}
	void ResourceManager::Update(float fDeltaTime)
	{
		std::shared_ptr<Camera> pCamera = Graphics::GetInst()->GetCamera();

		if (pCamera)
		{
			std::shared_ptr<Transform> pTransform = pCamera->GetTransform();

			if (pTransform)
			{
				const Vector3& vPos = pTransform->GetPosition();

				const Vector3& vVel = pTransform->GetVelocity();

				FMOD_VECTOR vListenerPos = { vPos.x, vPos.y, vPos.z };
				FMOD_VECTOR vListenerVel = { vVel.x, vVel.y, vVel.z };

				const Vector3& vAxisZ = pTransform->GetAxis(AXIS_TYPE::Z);

				FMOD_VECTOR _vAxisZ = { vAxisZ.x,vAxisZ.y, vAxisZ.z };

				const Vector3& vAxisY = pTransform->GetAxis(AXIS_TYPE::Y);

				FMOD_VECTOR _vAxisY = { vAxisY.x,vAxisY.y, vAxisY.z };

				m_pSoundSystem->set3DListenerAttributes(0, &vListenerPos, &vListenerVel, &_vAxisZ, &_vAxisY);
			}
		}

		m_pSoundSystem->update();
	}
}
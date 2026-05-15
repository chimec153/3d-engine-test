#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL ResourceManager
	{
	private:
		ResourceManager();
		~ResourceManager();

	private:
		static ResourceManager* m_pInst;

	public:
		static ResourceManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new ResourceManager;
			}

			return m_pInst;
		}

		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		std::unordered_map<std::string, std::shared_ptr<class Skeleton>>	m_mapSkeleton;
		std::unordered_map<std::string, std::shared_ptr<class Sequence>>	m_mapSequence;
		std::unordered_map<std::string, std::shared_ptr<class Animation>>	m_mapAnimation;
		std::unordered_map<std::string, std::shared_ptr<class Sound>>	m_mapSound;

	public:
		std::shared_ptr<Skeleton> CreateSkeleton(const std::string& strTag, const std::vector<BONE>& vecBone);
		std::shared_ptr<Skeleton> FindSkeleton(const std::string& strTag)	const;
		std::shared_ptr<Sequence> CreateSequence(const std::string& strTag);
		std::shared_ptr<Sequence> FindSequence(const std::string& strTag)	const;

		// Debug: dump registered skeleton / sequence tags to OutputDebugString.
		void DumpRegisteredTags()	const;
		std::shared_ptr<Animation> FindAnimation(const std::string& strTag)	const;
		void LoadFile(const TCHAR* pFilePath, const std::string& strPathKey = MESH_PATH);
		// Scan MATERIAL_PATH for *.mat assets and register each into
		// BindableManager<Material>. Called once at startup so meshes
		// loaded later can resolve material references by tag.
		void LoadAllMaterials();
		// Enumerate every registered Material — backs the editor's material
		// picker dropdown. Returns the BindableManager<Material> backing map
		// directly; live, no copy. (Also forces Engine.dll to instantiate
		// BindableManager<Material>::GetMap so the editor links against it.)
		const std::unordered_map<std::string, std::shared_ptr<class Material>>& GetAllMaterials() const;
		void LoadSkeleton(const char* pFilePath, const std::string& strPathKey = MESH_PATH);
		void LoadSequence(const char* pFilePath, const std::string& strPathKey = MESH_PATH);
		void LoadSequenceByTag(const std::string& strTag, const char* pFilePath, const std::string& strPathKey = MESH_PATH);
		std::shared_ptr<Sound> CreateSound(const std::string& strSound, const char* pFilePath, const std::string& strPathKey = SOUND_PATH, float fMin = 0.5f, float fMax = 5000.f, FMOD_MODE b3D = FMOD_3D, bool bLoop = false);
		std::shared_ptr<Sound> FindSound(const std::string& strSound)	const;
		void Play_Sound(const std::string& strSound);
		void Stop_Sound(const std::string& strSound);

	private:
		FMOD::System* m_pSoundSystem;

	public:
		bool Init();
		void Update(float fDeltaTime);
	};

}
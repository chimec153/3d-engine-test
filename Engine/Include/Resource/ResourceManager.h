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

	public:
		std::shared_ptr<Skeleton> CreateSkeleton(const std::string& strTag, const std::vector<BONE>& vecBone);
		std::shared_ptr<Skeleton> FindSkeleton(const std::string& strTag)	const;
		std::shared_ptr<Sequence> CreateSequence(const std::string& strTag);
		std::shared_ptr<Sequence> FindSequence(const std::string& strTag)	const;
		std::shared_ptr<Animation> FindAnimation(const std::string& strTag)	const;
		void LoadFile(const TCHAR* pFilePath, const std::string& strPathKey = MESH_PATH);
		void LoadSkeleton(const char* pFilePath, const std::string& strPathKey = MESH_PATH);
		void LoadSequence(const char* pFilePath, const std::string& strPathKey = MESH_PATH);
	};

}
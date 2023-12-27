#pragma once

#include "../Core/Ptr.h"
#include "Layer.h"

namespace Engine
{
	class ENGINE_DLL Scene
	{
	public:
		Scene();
		virtual ~Scene();

	private:
		std::list<std::shared_ptr<class Layer>>	m_LayerList;

	public:
		void AddLayer(const std::string& strTag, int iZOrder = 0);
		void AddLayer(std::shared_ptr<Layer> pLayer);
		std::shared_ptr<class Layer> FindLayer(const std::string& strTag)	const;

	public:
		template <typename T, typename ...Args>
		std::shared_ptr<T> CreateDrawable(const std::string& strTag, const class std::shared_ptr<class Layer>& pLayer, Args... args)
		{
			std::shared_ptr<T> pDrawable = std::make_shared<T>(args...);

			pDrawable->SetTag(strTag);

			pDrawable->SetScene(this);

			if (!pDrawable->Init())
			{
				return nullptr;
			}

			pLayer->AddDrawable(pDrawable);

			return pDrawable;
		}

	public:

		template <typename T, typename ...Args>
		static std::shared_ptr<T> CreateProtoType(const std::string& strTag, Args... args, SCENE_TYPE eSceneType = SCENE_TYPE::CURRENT)
		{
			std::shared_ptr<T> pDrawable = std::static_pointer_cast<T>(FindProtoType(strTag, eSceneType));

			if (pDrawable != nullptr)
			{
				return nullptr;
			}

			pDrawable = std::make_shared<T>(args...);

			pDrawable->SetTag(strTag);

			if (!pDrawable->Init())
			{
				return nullptr;
			}

			m_mapProtoType[static_cast<int>(eSceneType)].insert(std::make_pair(strTag, pDrawable));

			return pDrawable;
		}

		static std::shared_ptr<class Bindable> FindProtoType(const std::string& strTag, SCENE_TYPE eSceneType);

		std::shared_ptr<class Bindable> CreateCloneDrawable(const std::string& strTag, const std::string& strProto, const class std::shared_ptr<class Layer>& pLayer, SCENE_TYPE eSceneType = SCENE_TYPE::CURRENT);

	private:
		static std::unordered_map<std::string, std::shared_ptr<class Bindable>> m_mapProtoType[static_cast<int>(SCENE_TYPE::END)];

	public:
		static void Clear();

	public:
		virtual bool Init();
		virtual void Input(float fDeltaTime);
		virtual void Update(float fDeltaTime);
		virtual void FixedUpdate(float fDelatTime);
		virtual void PostUpdate(float fDeltaTime);
		virtual void Collision(float fDeltaTime);
		virtual void PreDraw(float fDeltaTime);
		virtual void Draw();

	public:
		void Save(FILE* pFile);
		void Load(FILE* pFile);
		void Save(const char* pFilePath, const std::string& strPath = ROOT_PATH);
		void Load(const char* pFilePath, const std::string& strPath = ROOT_PATH);
		void SaveFromFullPath(const char* pFullPath);
		void LoadFromFullPath(const char* pFullPath);
		void SaveFromFullPath(const TCHAR* pFullPath);
		void LoadFromFullPath(const TCHAR* pFullPath);

	public:
		std::shared_ptr<Bindable> FindBindable(const std::string& strTag)	const;
	};

}
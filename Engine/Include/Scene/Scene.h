#pragma once

#include "../Core/Ptr.h"
#include "Layer.h"

namespace Engine
{
	namespace RenderV2 { class Drawable; }

	class ENGINE_DLL Scene
	{
	public:
		Scene();
		virtual ~Scene();

	private:
		std::list<std::shared_ptr<class Layer>>	m_LayerList;
		std::list<std::shared_ptr<RenderV2::Drawable>> m_v2DrawableList;

	public:
		void AddLayer(const std::string& strTag, int iZOrder = 0);
		void AddLayer(std::shared_ptr<Layer> pLayer);
		std::shared_ptr<class Layer> FindLayer(const std::string& strTag)	const;

		// RenderV2 drawables live alongside the legacy Layer/Drawable system.
		// Scene::Update advances them every frame, Scene::PreDraw submits
		// them into RenderManager's V2 queue using the active camera.
		void AddV2Drawable(std::shared_ptr<RenderV2::Drawable> pDrawable);
		const std::list<std::shared_ptr<RenderV2::Drawable>>& GetV2Drawables() const { return m_v2DrawableList; }

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

			if (pLayer)
			{
				pLayer->AddDrawable(pDrawable);
			}

			return pDrawable;
		}

		// Phase B.5 — Component-side analogue of CreateDrawable. Used to
		// register top-level Components (Camera, Light controller, etc.) on
		// a Layer. Layer drives lifecycle on its m_ComponentList.
		template <typename T, typename ...Args>
		std::shared_ptr<T> CreateComponent(const std::string& strTag, const class std::shared_ptr<class Layer>& pLayer, Args... args)
		{
			std::shared_ptr<T> pComp = std::make_shared<T>(args...);

			pComp->SetTag(strTag);

			if (!pComp->Init())
			{
				return nullptr;
			}

			if (pLayer)
			{
				pLayer->AddComponent(std::static_pointer_cast<class Component>(pComp));
			}

			return pComp;
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
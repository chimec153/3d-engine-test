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
		// Phase E7 — CreateDrawable<T> removed. All entities now go through
		// CreateGameObject<T> (defined below); Component-only registration
		// uses CreateComponent<T>. The Drawable-typed factory had no live
		// callers after the Editor migration finished.

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

		// Phase E1 — entity (GameObject / Actor) creation. Scene-level
		// factory. The returned GameObject's Components are added by the
		// caller via AddComponent<T>. Layer drives lifecycle.
		template <typename T = class GameObject, typename ...Args>
		std::shared_ptr<T> CreateGameObject(const std::string& strTag, const class std::shared_ptr<class Layer>& pLayer, Args... args)
		{
			std::shared_ptr<T> pObj = std::make_shared<T>(args...);

			pObj->SetTag(strTag);

			// Phase E5 — set layer back-pointer BEFORE Init runs so the
			// GameObject's Init / its Components' Init can reach the Scene
			// via GetScene() (mirrors Drawable::GetScene access from the
			// Drawable era).
			if (pLayer)
				pObj->SetLayer(pLayer.get());

			if (!pObj->Init())
			{
				return nullptr;
			}

			if (pLayer)
			{
				pLayer->AddGameObject(std::static_pointer_cast<class GameObject>(pObj));
			}

			return pObj;
		}

	public:
		// Phase E7 — CreateProtoType / FindProtoType / CreateCloneDrawable
		// and the static m_mapProtoType registry are removed. The clone-
		// from-prototype path was Drawable-typed and last used by the
		// editor's "Player" navmesh-spawn UI, which now spawns GameObjects
		// directly via CreateGameObject.

	private:

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
	};

}
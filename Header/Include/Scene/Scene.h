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
		const std::list<std::shared_ptr<class Layer>>& GetLayerList() const { return m_LayerList; }

	public:
		// Phase E7 — CreateDrawable<T> and CreateComponent<T> removed. All
		// entities now go through CreateGameObject<T> (defined below); top-
		// level Components (Camera, Light, Mouse) are hosted on a wrapper
		// GameObject created via CreateGameObject + AddComponent<T>.

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
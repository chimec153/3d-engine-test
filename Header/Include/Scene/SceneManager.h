#pragma once

#include "../Core/Window.h"
#include "Layer.h"

namespace Engine
{
	class ENGINE_DLL SceneManager
	{
	private:
		SceneManager();
		~SceneManager();

	private:
		static SceneManager* m_pInst;

	public:
		static SceneManager* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new SceneManager;
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
		class Scene* m_pScene;
		class Scene* m_pNextScene;

	public:
		void ChangeScene();
		template <typename T>
		T* CreateScene(SCENE_TYPE type = SCENE_TYPE::NEXT)
		{
			T* pScene = dbg_new T;

			if (!pScene->Init())
			{
				SAFE_DELETE(pScene);
				return nullptr;
			}

			switch (type)
			{
			case SCENE_TYPE::CURRENT:
				SAFE_DELETE(m_pScene);
				m_pScene = pScene;
				break;
			case SCENE_TYPE::NEXT:
				SAFE_DELETE(m_pNextScene);
				m_pNextScene = pScene;
				break;
			}

			return static_cast<T*>(pScene);
		}
		Scene* GetScene(SCENE_TYPE type = SCENE_TYPE::CURRENT)	const;

		// Runtime equivalent of CreateScene<T>(). Use when the concrete type
		// is not visible at compile time (e.g. created via SceneFactory in
		// Editor after LoadLibrary). Takes ownership; calls Init().
		bool SetScene(Scene* pScene, SCENE_TYPE type = SCENE_TYPE::NEXT);

		// Drops current + pending scenes (and the GameObjects/Components
		// they own). Used by ProjectModule before LoadLibrary so a fresh
		// project starts without stale entities from the previous world.
		void ClearScene();

	public:
		bool Input(float fDeltaTime);
		bool Update(float fDeltaTime);
		void FixedUpdate(float fDelatTime);
		bool PostUpdate(float fDeltaTime);
		bool Collision(float fDeltaTime);
		void PreDraw(float fDeltaTime);
		void Draw();
	};

}
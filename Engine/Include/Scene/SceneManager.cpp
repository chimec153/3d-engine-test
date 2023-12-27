#include "SceneManager.h"
#include "Scene.h"
#include "Layer.h"
#include "../Input/Input.h"

namespace Engine
{
	SceneManager* SceneManager::m_pInst = nullptr;

	SceneManager::SceneManager() :
		m_pScene(nullptr)
		, m_pNextScene(nullptr)
	{
	}

	SceneManager::~SceneManager()
	{
		SAFE_DELETE(m_pScene);
		SAFE_DELETE(m_pNextScene);
	}

	void SceneManager::ChangeScene()
	{
		SAFE_DELETE(m_pScene);

		m_pScene = m_pNextScene;

		m_pNextScene = nullptr;

		CInput::GetInst()->SceneChanged();
	}

	Scene* SceneManager::GetScene(SCENE_TYPE type) const
	{
		switch (type)
		{
		case SCENE_TYPE::CURRENT:
			return m_pScene;
		case SCENE_TYPE::NEXT:
			return m_pNextScene;
		}

		assert(false);

		return nullptr;
	}

	bool SceneManager::Input(float fDeltaTime)
	{
		if (m_pNextScene)
		{
			ChangeScene();
			return false;
		}

		m_pScene->Input(fDeltaTime);

		return true;
	}

	bool SceneManager::Update(float fDeltaTime)
	{
		if (m_pNextScene)
		{
			ChangeScene();
			return false;
		}

		m_pScene->Update(fDeltaTime);

		return true;
	}

	void SceneManager::FixedUpdate(float fDeltaTime)
	{
		m_pScene->FixedUpdate(fDeltaTime);
	}

	bool SceneManager::PostUpdate(float fDeltaTime)
	{
		if (m_pNextScene)
		{
			ChangeScene();
			return false;
		}

		m_pScene->PostUpdate(fDeltaTime);

		return true;
	}

	bool SceneManager::Collision(float fDeltaTime)
	{
		if (m_pNextScene)
		{
			ChangeScene();
			return false;
		}

		m_pScene->Collision(fDeltaTime);

		return true;
	}

	void SceneManager::PreDraw(float fDeltaTime)
	{
		m_pScene->PreDraw(fDeltaTime);
	}

	void SceneManager::Draw()
	{
		m_pScene->Draw();
	}
}
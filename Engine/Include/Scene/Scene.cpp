#include "Scene.h"
#include "Layer.h"
#include "../Bindable/Drawable.h"
#include "SceneManager.h"
#include "../Core/PathManager.h"

namespace Engine
{
	std::unordered_map<std::string, std::shared_ptr<Bindable>> Scene::m_mapProtoType[static_cast<int>(SCENE_TYPE::END)] = {};

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	void Scene::AddLayer(const std::string& strTag, int iZOrder)
	{
		std::shared_ptr<Layer> pLayer = std::make_shared<Layer>();

		pLayer->SetTag(strTag);

		pLayer->SetZOrder(iZOrder);

		AddLayer(pLayer);
	}

	void Scene::AddLayer(std::shared_ptr<Layer> pLayer)
	{
		pLayer->SetScene(this);

		m_LayerList.push_back(pLayer);

		m_LayerList.sort([](const std::shared_ptr<Layer>& src, const std::shared_ptr<Layer>& dest)->bool
			{
				return src->GetZOrder() < dest->GetZOrder();
			}
		);
	}

	std::shared_ptr<Layer> Scene::FindLayer(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Layer>>::const_iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::const_iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetTag() == strTag)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Layer>();
	}

	void Scene::Clear()
	{
		for (int i = 0; i < static_cast<int>(SCENE_TYPE::END); ++i)
		{
			m_mapProtoType[i].clear();
		}
	}

	bool Scene::Init()
	{
		AddLayer(DEFAULT_LAYER);
		AddLayer(ALPHA_LAYER, 1);
		AddLayer(UI_LAYER, 2);

		return true;
	}

	void Scene::Input(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Input(fDeltaTime);
			++iter;
		}
	}

	void Scene::Update(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Update(fDeltaTime);
			++iter;
		}
	}

	void Scene::FixedUpdate(float fDelatTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->FixedUpdate(fDelatTime);
			++iter;
		}
	}

	void Scene::PostUpdate(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->PostUpdate(fDeltaTime);
			++iter;
		}
	}

	void Scene::Collision(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Collision(fDeltaTime);
			++iter;
		}
	}

	void Scene::PreDraw(float fDeltaTime)
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->PreDraw(fDeltaTime);
			++iter;
		}
	}

	void Scene::Draw()
	{
		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_LayerList.erase(iter);
				iterEnd = m_LayerList.end();
				continue;
			}

			else if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}

			(*iter)->Draw();
			++iter;
		}
	}

	void Scene::Save(FILE* pFile)
	{
		int iLayerCount =static_cast<int>(m_LayerList.size());

		fwrite(&iLayerCount, 4, 1, pFile);

		std::list<std::shared_ptr<Layer>>::iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->Save(pFile);
		}
	}

	void Scene::Load(FILE* pFile)
	{
		int iLayerCount = 0;

		fread(&iLayerCount, 4, 1, pFile);

		for (int i = 0; i < iLayerCount; ++i)
		{
			std::shared_ptr<Layer> pLayer = std::make_shared<Layer>();

			AddLayer(pLayer);

			pLayer->Load(pFile);

			m_LayerList.sort([](const std::shared_ptr<Layer>& src, const std::shared_ptr<Layer>& dest)->bool
				{
					return src->GetZOrder() < dest->GetZOrder();
				}
			);
		}
	}

	void Scene::Save(const char* pFilePath, const std::string& strPath)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPath);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		SaveFromFullPath(strFullPath);
	}

	void Scene::Load(const char* pFilePath, const std::string& strPath)
	{
		char strFullPath[MAX_PATH] = {};

		const char* pPath = CPathManager::GetInst()->FindMultibytePath(strPath);

		if (pPath)
		{
			strcpy_s(strFullPath, pPath);
		}

		strcat_s(strFullPath, pFilePath);

		LoadFromFullPath(strFullPath);
	}

	void Scene::SaveFromFullPath(const char* pFullPath)
	{
		FILE* pFile = nullptr;

		fopen_s(&pFile, pFullPath, "wb");

		if (pFile)
		{
			Save(pFile);

			fclose(pFile);
		}
	}

	void Scene::LoadFromFullPath(const char* pFullPath)
	{
		FILE* pFile = nullptr;

		fopen_s(&pFile, pFullPath, "rb");

		if (pFile)
		{
			Load(pFile);

			fclose(pFile);
		}
	}

	void Scene::SaveFromFullPath(const TCHAR* pFullPath)
	{
		char strFullPath[MAX_PATH] = {};

		WideCharToMultiByte(CP_ACP, 0, pFullPath, -1, strFullPath, MAX_PATH, nullptr, nullptr);

		SaveFromFullPath(strFullPath);
	}

	void Scene::LoadFromFullPath(const TCHAR* pFullPath)
	{
		char strFullPath[MAX_PATH] = {};

		WideCharToMultiByte(CP_ACP, 0, pFullPath, -1, strFullPath, MAX_PATH, nullptr, nullptr);

		LoadFromFullPath(strFullPath);
	}

	std::shared_ptr<Bindable> Scene::FindBindable(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Layer>>::const_iterator iter = m_LayerList.begin();
		std::list<std::shared_ptr<Layer>>::const_iterator iterEnd = m_LayerList.end();

		for (; iter != iterEnd; ++iter)
		{
			std::shared_ptr<Bindable> pBindable = (*iter)->FindDrawable(strTag);

			if (pBindable)
			{
				return pBindable;
			}
		}

		return std::shared_ptr<Bindable>();
	}


	ENGINE_DLL std::shared_ptr<Bindable> Scene::FindProtoType(const std::string& strTag, SCENE_TYPE eSceneType)
	{
		std::unordered_map<std::string, std::shared_ptr<Bindable>>::iterator iter = m_mapProtoType[static_cast<int>(eSceneType)].find(strTag);

		if (iter == m_mapProtoType[static_cast<int>(eSceneType)].end())
		{
			return std::shared_ptr<Bindable>();
		}

		return iter->second;
	}

	std::shared_ptr<Bindable> Scene::CreateCloneDrawable(const std::string& strTag, const std::string& strProto, const class std::shared_ptr<Layer>& pLayer, SCENE_TYPE eSceneType)
	{
		const std::shared_ptr<Bindable>& pProtoType = FindProtoType(strProto, eSceneType);

		if (pProtoType == nullptr)
		{
			return nullptr;
		}

		const std::shared_ptr<Bindable>& pClone = pProtoType->Clone();

		pClone->SetTag(strTag);

		pLayer->AddDrawable(pClone);

		pClone->SetScene(this);

		return pClone;
	}
}
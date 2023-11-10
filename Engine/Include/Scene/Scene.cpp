#include "Scene.h"
#include "Layer.h"
#include "../Bindable/Drawable.h"
#include "SceneManager.h"

namespace Engine
{
	std::unordered_map<std::string, std::shared_ptr<Bindable>> Scene::m_mapProtoType[static_cast<int>(SCENE_TYPE::END)] = {};

	Scene::Scene()
	{
		AddLayer(DEFAULT_LAYER);
		AddLayer(ALPHA_LAYER, 1);
	}

	Scene::~Scene()
	{
	}

	void Scene::AddLayer(const std::string& strTag, int iZOrder)
	{
		std::shared_ptr<Layer> pLayer = std::make_shared<Layer>();

		pLayer->SetTag(strTag);

		pLayer->SetZOrder(iZOrder);

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
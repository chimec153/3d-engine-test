#include "Layer.h"
#include "../Bindable/Drawable.h"
#include "../Component/Component.h"
#include "../GameObject/GameObject.h"
#include "../Core/PathManager.h"
#include "../Scene/Scene.h"
#include "../Thread/ThreadManager.h"
#include "../Thread/LoadingThread.h"

namespace Engine
{
	Layer::Layer() :
		m_iZOrder(0)
		, m_pScene(nullptr)
	{
	}

	Layer::~Layer()
	{
	}

	void Layer::SetZOrder(int iZOrder)
	{
		m_iZOrder = iZOrder;
	}

	int Layer::GetZOrder() const
	{
		return m_iZOrder;
	}

	void Layer::AddDrawable(const std::shared_ptr<Bindable>& pDrawable)
	{
		pDrawable->SetLayer(this);

		pDrawable->SetScene(m_pScene);

		pDrawable->Start();

		m_DrawList.push_back(pDrawable);
	}

	void Layer::AddComponent(const std::shared_ptr<Component>& pComp)
	{
		pComp->Start();
		m_ComponentList.push_back(pComp);
	}

	void Layer::AddGameObject(const std::shared_ptr<GameObject>& pObj)
	{
		pObj->Start();
		m_GameObjectList.push_back(pObj);
	}

	const std::list<std::shared_ptr<GameObject>>& Layer::GetGameObjectList() const
	{
		return m_GameObjectList;
	}

	std::shared_ptr<GameObject> Layer::FindGameObject(const std::string& strTag) const
	{
		for (const auto& p : m_GameObjectList)
		{
			if (p->GetTag() == strTag) return p;
		}
		return nullptr;
	}

	void Layer::DeleteGameObject(std::shared_ptr<GameObject> pObj)
	{
		for (auto iter = m_GameObjectList.begin(); iter != m_GameObjectList.end(); ++iter)
		{
			if (*iter == pObj)
			{
				m_GameObjectList.erase(iter);
				return;
			}
		}
	}

	void Layer::SetScene(Scene* pScene)
	{
		m_pScene = pScene;
	}

	const std::list<class std::shared_ptr<class Bindable>>& Layer::GetDrawList() const
	{
		return m_DrawList;
	}

	const std::list<std::shared_ptr<Component>>& Layer::GetComponentList() const
	{
		return m_ComponentList;
	}

	std::shared_ptr<Component> Layer::FindComponent(const std::string& strTag) const
	{
		for (const auto& p : m_ComponentList)
		{
			if (p->GetTag() == strTag) return p;
			if (auto pChild = p->FindChild(strTag)) return pChild;
		}
		return nullptr;
	}

	std::shared_ptr<Component> Layer::FindComponent(COMPONENT_TYPE eType) const
	{
		for (const auto& p : m_ComponentList)
		{
			if (p->GetComponentType() == eType) return p;
		}
		return nullptr;
	}

	void Layer::DeleteComponent(std::shared_ptr<Component> pComp)
	{
		for (auto iter = m_ComponentList.begin(); iter != m_ComponentList.end(); ++iter)
		{
			if (*iter == pComp)
			{
				m_ComponentList.erase(iter);
				return;
			}
		}
	}

	const std::shared_ptr<class LoadingThread>& Layer::GetLoadingThread() const
	{
		return m_pLoadingThread;
	}

	std::shared_ptr<Bindable> Layer::FindDrawable(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetTag() == strTag)
			{
				return *iter;
			}

			std::shared_ptr<Bindable> pChild = (*iter)->FindChild(strTag);

			if (pChild)
			{
				return pChild;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	std::shared_ptr<Bindable> Layer::FindDrawable(BINDABLE_TYPE eType) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetBindableType() == eType)
			{
				return *iter;
			}

			std::shared_ptr<Bindable> pChild = (*iter)->FindChild(eType);

			if (pChild)
			{
				return pChild;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Layer::DeleteDrawable(std::shared_ptr<Bindable> pDrawable)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd; ++iter)
		{
			if (*iter == pDrawable)
			{
				m_DrawList.erase(iter);

				return;
			}
		}
	}

	namespace
	{
		// Phase B.5 — shared lifecycle iteration over a typed list. Same
		// active/enable filter pattern Bindable/Drawable use.
		template <typename ListT, typename Fn>
		void ForEachActive(ListT& list, Fn fn)
		{
			for (auto iter = list.begin(); iter != list.end();)
			{
				if (!(*iter)->IsActive())
				{
					iter = list.erase(iter);
					continue;
				}
				if (!(*iter)->IsEnable())
				{
					++iter;
					continue;
				}
				fn(*iter);
				++iter;
			}
		}
	}

	void Layer::Input(float fDeltaTime)
	{
		ForEachActive(m_DrawList,       [&](const auto& p) { p->Input(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->Input(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Input(fDeltaTime); });
	}

	void Layer::Update(float fDeltaTime)
	{
		if (m_pLoadingThread)
		{
			if (m_pLoadingThread->IsFinish())
			{
				AddDrawable(m_pLoadingThread->GetDrawable());

				m_pLoadingThread = nullptr;
			}
		}

		ForEachActive(m_DrawList,       [&](const auto& p) { p->Update(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->Update(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Update(fDeltaTime); });
	}

	void Layer::FixedUpdate(float fDeltaTime)
	{
		ForEachActive(m_DrawList,       [&](const auto& p) { p->FixedUpdate(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->FixedUpdate(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->FixedUpdate(fDeltaTime); });
	}

	void Layer::Collision(float fDeltaTime)
	{
		ForEachActive(m_DrawList,       [&](const auto& p) { p->Collision(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->Collision(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Collision(fDeltaTime); });
	}

	void Layer::PostUpdate(float fDeltaTime)
	{
		ForEachActive(m_DrawList,       [&](const auto& p) { p->PostUpdate(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->PostUpdate(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->PostUpdate(fDeltaTime); });
	}

	void Layer::PreDraw(float fDeltaTime)
	{
		ForEachActive(m_DrawList,       [&](const auto& p) { p->PreDraw(fDeltaTime); });
		ForEachActive(m_ComponentList,  [&](const auto& p) { p->PreDraw(fDeltaTime); });
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->PreDraw(fDeltaTime); });
	}

	void Layer::Draw()
	{
	}
	void Layer::CreateLoadingThread(const TCHAR* pFullPath)
	{
		if (!m_pLoadingThread)
		{
			m_pLoadingThread = std::static_pointer_cast<LoadingThread>(ThreadManager::GetInst()->FindThread("Loading"));

			if (!m_pLoadingThread)
			{
				m_pLoadingThread = ThreadManager::GetInst()->CreateThread<LoadingThread>("Loading");
			}
			else
			{
				m_pLoadingThread->Init();
			}

			m_pLoadingThread->SetFullPath(pFullPath);

			m_pLoadingThread->Start();
		}
	}
	void Layer::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_iZOrder, 4, 1, pFile);

		int iBindableCount = static_cast<int>(m_DrawList.size());

		fwrite(&iBindableCount, 4, 1, pFile);

		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd; ++iter)
		{
			BINDABLE_TYPE eType = (*iter)->GetBindableType();

			fwrite(&eType, 4, 1, pFile);

			(*iter)->Save(pFile);
		}
	}
	void Layer::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_iZOrder, 4, 1, pFile);

		int iBindableCount = 0;

		fread(&iBindableCount, 4, 1, pFile);

		for (int i = 0; i < iBindableCount; ++i)
		{
			BINDABLE_TYPE eType = BINDABLE_TYPE::NONE;

			fread(&eType, 4, 1, pFile);

			std::shared_ptr<Bindable> pBindable = Bindable::CreateBindable(eType);

			if (!pBindable)
			{
				int iLength = 0;

				fread(&iLength, 4, 1, pFile);

				if (iLength)
				{
					std::unique_ptr<char[]> strBind = std::make_unique<char[]>(iLength + 1);

					strBind[iLength] = 0;

					fread(strBind.get(), 1, iLength, pFile);

					pBindable = Bindable::FindBindable(eType, strBind.get());
				}
			}
			else
			{
				pBindable->SetScene(m_pScene);

				pBindable->SetLayer(this);

				pBindable->Load(pFile);
			}

			assert(pBindable);

			AddDrawable(pBindable);
		}
	}
}
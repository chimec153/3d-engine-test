#include "Layer.h"
#include "../Bindable/Drawable.h"
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

		pDrawable->Start();

		m_DrawList.push_back(pDrawable);
	}

	void Layer::SetScene(Scene* pScene)
	{
		m_pScene = pScene;
	}

	const std::list<class std::shared_ptr<class Bindable>>& Layer::GetDrawList() const
	{
		return m_DrawList;
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
		}

		return std::shared_ptr<Bindable>();
	}

	void Layer::Input(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_DrawList.erase(iter);
				iterEnd = m_DrawList.end();
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

		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_DrawList.erase(iter);
				iterEnd = m_DrawList.end();
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

	void Layer::Collision(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_DrawList.erase(iter);
				iterEnd = m_DrawList.end();
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

	void Layer::PreDraw(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_DrawList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_DrawList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_DrawList.erase(iter);
				iterEnd = m_DrawList.end();
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
}
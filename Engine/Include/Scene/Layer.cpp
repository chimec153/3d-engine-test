#include "Layer.h"
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

	void Layer::AddGameObject(const std::shared_ptr<GameObject>& pObj)
	{
		// Phase E5 — wire up layer back-pointer (idempotent: Scene's
		// CreateGameObject already does this pre-Init so the GameObject's
		// Init/components can call GetScene; this catches the case where
		// AddGameObject is invoked outside of CreateGameObject).
		pObj->SetLayer(this);

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

	const std::shared_ptr<class LoadingThread>& Layer::GetLoadingThread() const
	{
		return m_pLoadingThread;
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
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Input(fDeltaTime); });
	}

	void Layer::Update(float fDeltaTime)
	{
		if (m_pLoadingThread)
		{
			if (m_pLoadingThread->IsFinish())
			{
				// Phase E7 — LoadingThread now produces a GameObject (with
				// MeshRendererComponent populated by MeshLoader) instead of
				// a Drawable.
				if (auto pObj = m_pLoadingThread->GetGameObject())
					AddGameObject(pObj);

				m_pLoadingThread = nullptr;
			}
		}

		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Update(fDeltaTime); });
	}

	void Layer::FixedUpdate(float fDeltaTime)
	{
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->FixedUpdate(fDeltaTime); });
	}

	void Layer::Collision(float fDeltaTime)
	{
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->Collision(fDeltaTime); });
	}

	void Layer::PostUpdate(float fDeltaTime)
	{
		ForEachActive(m_GameObjectList, [&](const auto& p) { p->PostUpdate(fDeltaTime); });
	}

	void Layer::PreDraw(float fDeltaTime)
	{
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

		// Phase E7 — Drawable serialization removed. m_DrawList no longer
		// holds live entities (game classes migrated to GameObject), so
		// iterating it would always write zero. The "count = 0" placeholder
		// stays so existing .scn files saved before the migration can still
		// be read past this Layer header without going off-rails. GameObject
		// serialization is intentionally not added here — scenes are
		// reauthored from code today; a proper entity-graph format will be
		// designed when scene-from-disk gets reintroduced.
		const int iEntityCount = 0;
		fwrite(&iEntityCount, 4, 1, pFile);
	}
	void Layer::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_iZOrder, 4, 1, pFile);

		// Phase E7 — match Save: read the count placeholder and bail.
		// Old scene files with non-zero counts can no longer be parsed
		// (Drawable construction path was the only consumer); any such
		// files need to be re-saved with the new (empty) format.
		int iEntityCount = 0;
		fread(&iEntityCount, 4, 1, pFile);
		assert(iEntityCount == 0 && "Legacy Drawable-serialized scene; resave with the post-migration engine.");
	}
}
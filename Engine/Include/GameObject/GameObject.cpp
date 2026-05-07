#include "GameObject.h"
#include "../Scene/Layer.h"

namespace Engine
{
	GameObject::GameObject() :
		m_pParent(nullptr)
		, m_pLayer(nullptr)
	{
	}

	GameObject::GameObject(const GameObject& other) :
		CRef(other)
		, m_pParent(nullptr)
		, m_pLayer(other.m_pLayer)
	{
		// Components are deep-cloned so each entity has its own state.
		for (const auto& p : other.m_Components)
		{
			auto cloned = p->Clone();
			if (cloned)
				m_Components.push_back(cloned);
		}
	}

	GameObject::~GameObject() = default;

	Scene* GameObject::GetScene() const
	{
		return m_pLayer ? m_pLayer->GetScene() : nullptr;
	}

	void GameObject::AddComponent(const std::shared_ptr<Component>& pComp)
	{
		if (!pComp) return;
		// Phase E5 — wire up owner so the Component can reach the entity
		// via GetGameObjectOwner. Mirrors the templated AddComponent
		// overload's behavior for pre-constructed Components.
		pComp->SetGameObjectOwner(this);
		m_Components.push_back(pComp);
	}

	std::shared_ptr<Component> GameObject::FindComponent(COMPONENT_TYPE eType) const
	{
		for (const auto& p : m_Components)
		{
			if (p->GetComponentType() == eType)
				return p;
		}
		return nullptr;
	}

	std::shared_ptr<Component> GameObject::FindComponent(const std::string& strTag) const
	{
		for (const auto& p : m_Components)
		{
			if (p->GetTag() == strTag)
				return p;
		}
		return nullptr;
	}

	const std::list<std::shared_ptr<Component>>& GameObject::GetComponentList() const
	{
		return m_Components;
	}

	GameObject* GameObject::GetParent() const
	{
		return m_pParent;
	}

	void GameObject::SetParent(GameObject* pParent)
	{
		if (m_pParent == pParent) return;

		// Detach from old parent's child list.
		if (m_pParent)
		{
			auto& siblings = const_cast<std::list<GameObject*>&>(m_pParent->m_Children);
			siblings.remove(this);
		}

		m_pParent = pParent;

		if (m_pParent)
		{
			m_pParent->m_Children.push_back(this);
		}
	}

	const std::list<GameObject*>& GameObject::GetChildren() const
	{
		return m_Children;
	}

	bool GameObject::Init()
	{
		return true;
	}

	void GameObject::Start() {}

	namespace
	{
		// Phase E1 — active/enable iteration pattern. Renamed from the
		// generic ForEachActive used in Layer.cpp because the unity build
		// puts both translation units in the same TU and would collide.
		template <typename ListT, typename Fn>
		void ForEachActiveGameObjectComp(ListT& list, Fn fn)
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

	void GameObject::Input(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->Input(fDeltaTime); });
	}

	void GameObject::Update(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->Update(fDeltaTime); });
	}

	void GameObject::FixedUpdate(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->FixedUpdate(fDeltaTime); });
	}

	void GameObject::Collision(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->Collision(fDeltaTime); });
	}

	void GameObject::PostUpdate(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->PostUpdate(fDeltaTime); });
	}

	void GameObject::PreDraw(float fDeltaTime)
	{
		ForEachActiveGameObjectComp(m_Components, [&](const auto& p) { p->PreDraw(fDeltaTime); });
	}

	void GameObject::Save(FILE* pFile)
	{
		__super::Save(pFile);

		int iCount = static_cast<int>(m_Components.size());
		fwrite(&iCount, 4, 1, pFile);

		for (const auto& p : m_Components)
		{
			COMPONENT_TYPE eType = p->GetComponentType();
			fwrite(&eType, 4, 1, pFile);
			p->Save(pFile);
		}
	}

	void GameObject::Load(FILE* pFile)
	{
		__super::Load(pFile);

		// Phase E1 — Component reconstruction deferred. Need a
		// CreateComponent(COMPONENT_TYPE) factory similar to
		// Bindable::CreateBindable. Will land alongside the first concrete
		// E4 game-class migration. For now Save writes the count and Load
		// just consumes it without reconstructing.
		int iCount = 0;
		fread(&iCount, 4, 1, pFile);
	}
}

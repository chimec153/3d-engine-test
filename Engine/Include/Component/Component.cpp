#include "Component.h"
#include "../Bindable/Drawable.h"
#include "../Bindable/Transform.h"
#include "../GameObject/GameObject.h"

namespace Engine
{
	Component::Component() :
		m_eComponentType(COMPONENT_TYPE::NONE)
		, m_pParent(nullptr)
		, m_pGameObjectOwner(nullptr)
	{
	}

	Component::Component(const Component& comp) :
		CRef(comp)
		, m_eComponentType(comp.m_eComponentType)
		, m_pParent(nullptr)
		, m_pGameObjectOwner(nullptr)
	{
		for (const auto& pChild : comp.m_ChildList)
		{
			AddChild(pChild->Clone());
		}
	}

	Component::~Component() noexcept
	{
	}

	void Component::SetComponentType(COMPONENT_TYPE eType)
	{
		m_eComponentType = eType;
	}

	COMPONENT_TYPE Component::GetComponentType() const
	{
		return m_eComponentType;
	}

	Component* Component::GetParent() const
	{
		return m_pParent;
	}

	void Component::SetParent(Component* pParent)
	{
		m_pParent = pParent;
	}

	GameObject* Component::GetGameObjectOwner() const
	{
		return m_pGameObjectOwner;
	}

	void Component::SetGameObjectOwner(GameObject* pOwner)
	{
		m_pGameObjectOwner = pOwner;
	}

	std::shared_ptr<Transform> Component::GetHostTransform() const
	{
		if (m_pGameObjectOwner) return m_pGameObjectOwner->GetComponent<Transform>();
		return nullptr;
	}

	const std::list<std::shared_ptr<Component>>& Component::GetChildList() const
	{
		return m_ChildList;
	}

	void Component::AddChild(const std::shared_ptr<Component>& pChild)
	{
		pChild->SetParent(this);
		m_ChildList.push_back(pChild);
	}

	std::shared_ptr<Component> Component::FindChild(COMPONENT_TYPE eType) const
	{
		for (const auto& pChild : m_ChildList)
		{
			if (pChild->GetComponentType() == eType)
				return pChild;
		}
		return nullptr;
	}

	std::shared_ptr<Component> Component::FindChild(const std::string& strTag) const
	{
		for (const auto& pChild : m_ChildList)
		{
			if (pChild->GetTag() == strTag)
				return pChild;

			std::shared_ptr<Component> p = pChild->FindChild(strTag);
			if (p)
				return p;
		}
		return nullptr;
	}

	void Component::DeleteChild(std::shared_ptr<Component> pComp)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end(); ++iter)
		{
			if (*iter == pComp)
			{
				(*iter)->SetParent(nullptr);
				m_ChildList.erase(iter);
				return;
			}
		}
	}

	bool Component::Init()
	{
		return true;
	}

	void Component::Start() {}

	void Component::Input(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->Input(fDeltaTime);
			++iter;
		}
	}

	void Component::Update(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->Update(fDeltaTime);
			++iter;
		}
	}

	void Component::FixedUpdate(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->FixedUpdate(fDeltaTime);
			++iter;
		}
	}

	void Component::Collision(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->Collision(fDeltaTime);
			++iter;
		}
	}

	void Component::PostUpdate(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->PostUpdate(fDeltaTime);
			++iter;
		}
	}

	void Component::PreDraw(float fDeltaTime)
	{
		for (auto iter = m_ChildList.begin(); iter != m_ChildList.end();)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				continue;
			}
			if (!(*iter)->IsEnable())
			{
				++iter;
				continue;
			}
			(*iter)->PreDraw(fDeltaTime);
			++iter;
		}
	}

	void Component::Reset() {}

	void Component::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_eComponentType, 4, 1, pFile);

		int iChildCount = static_cast<int>(m_ChildList.size());
		fwrite(&iChildCount, 4, 1, pFile);

		for (const auto& pChild : m_ChildList)
		{
			COMPONENT_TYPE eType = pChild->GetComponentType();
			fwrite(&eType, 4, 1, pFile);
			pChild->Save(pFile);
		}
	}

	void Component::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_eComponentType, 4, 1, pFile);

		// Child reconstruction: deferred to a future migration phase. Once
		// concrete Component types start migrating from Bindable, add a
		// Component::CreateComponent(COMPONENT_TYPE) factory similar to
		// Bindable::CreateBindable. For now, Save writes nothing migratable
		// and Load is a no-op past the header (no concrete Component subtype
		// exists yet).
		int iChildCount = 0;
		fread(&iChildCount, 4, 1, pFile);
	}
}

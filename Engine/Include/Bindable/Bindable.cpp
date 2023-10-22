#include "Bindable.h"
#include "../Core/Graphics.h"
#include "TransformBuffer.h"
#include "Agent.h"
#include "Drawable.h"

namespace Engine
{
	Bindable::Bindable() :
		m_eBindableType(BINDABLE_TYPE::NONE)
		, m_eObjectType(OBJECT_TYPE::BIND)
		, m_pParent(nullptr)
		, m_pScene(nullptr)
		, m_pLayer(nullptr)
	{
	}

	Bindable::Bindable(const Bindable& bindable) :
		CRef(bindable)
		, m_eBindableType(bindable.m_eBindableType)
		, m_eObjectType(bindable.m_eObjectType)
		, m_pParent(nullptr)
		, m_pScene(nullptr)
		, m_pLayer(nullptr)
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = bindable.m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = bindable.m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetBindableType())
			{
			case Engine::BINDABLE_TYPE::MATERIAL:
			case Engine::BINDABLE_TYPE::TRANSFORM:
			case BINDABLE_TYPE::AGENT:
			case BINDABLE_TYPE::ANIMATION:
				AddChild((*iter)->Clone());
				continue;
			} 

			switch ((*iter)->GetObjectType())
			{
			case Engine::OBJECT_TYPE::BIND:
				Bindable::AddChild(*iter);
				break;
			case Engine::OBJECT_TYPE::DRAW:
			case Engine::OBJECT_TYPE::COLLIDER:
				Bindable::AddChild((*iter)->Clone());
				break;
			}
		}
	}

	Bindable::~Bindable()
	{
	}


	void Bindable::SetBindableType(BINDABLE_TYPE eType)
	{
		m_eBindableType = eType;
	}

	BINDABLE_TYPE Bindable::GetBindableType() const
	{
		return m_eBindableType;
	}

	Bindable* Bindable::GetParent() const
	{
		return m_pParent;
	}

	void Bindable::SetParent(Bindable* pParent)
	{
		m_pParent = pParent;
	}

	const std::list<std::shared_ptr<Bindable>>& Bindable::GetChildList() const
	{
		return m_ChildList;
	}

	void Bindable::AddChild(const std::shared_ptr<class Bindable>& pChild)
	{
		Bindable* pParent = m_pParent;

		while (pParent)
		{
			if (pParent->GetObjectType() == OBJECT_TYPE::DRAW)
			{
				static_cast<Drawable*>(pParent)->AddDrawable(pChild);

				break;
			}

			pParent = m_pParent->GetParent();
		}

		pChild->SetParent(this);

		m_ChildList.push_back(pChild);
	}

	void Bindable::SetObjectType(OBJECT_TYPE eType)
	{
		m_eObjectType = eType;
	}

	OBJECT_TYPE Bindable::GetObjectType() const
	{
		return m_eObjectType;
	}


	std::shared_ptr<Bindable> Bindable::FindChild(BINDABLE_TYPE eType) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetBindableType() == eType)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::FindChilds(BINDABLE_TYPE eType, std::vector<std::shared_ptr<Bindable>>& vecBindables) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetBindableType() == eType)
			{
				vecBindables.push_back(*iter);
			}
		}
	}

	std::shared_ptr<Bindable> Bindable::FindChild(const std::string& strTag) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetTag() == strTag)
			{
				return *iter;
			}

			std::shared_ptr<Bindable> pChild = (*iter)->FindChild(strTag);

			if (pChild != nullptr)
			{
				return pChild;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::DeleteChild(const Bindable* const pBindable)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter).get() == pBindable)
			{
				m_ChildList.erase(iter);
				return;
			}
		}
	}

	std::shared_ptr<Bindable> Bindable::FindChild(OBJECT_TYPE eType) const
	{
		std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			if ((*iter)->GetObjectType() == eType)
			{
				return *iter;
			}
		}

		return std::shared_ptr<Bindable>();
	}

	void Bindable::SetScene(Scene* pScene)
	{
		m_pScene = pScene;
	}

	void Bindable::SetLayer(Layer* pLayer)
	{
		m_pLayer = pLayer;
	}

	Scene* Bindable::GetScene() const
	{
		return m_pScene;
	}

	bool Bindable::Init()
	{
		return true;
	}

	void Bindable::Start()
	{
	}

	void Bindable::Input(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Input(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::Update(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Update(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::Collision(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iterC = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterCEnd = m_ChildList.end();

		for (; iterC != iterCEnd;)
		{
			if (!(*iterC)->IsActive())
			{
				iterC = m_ChildList.erase(iterC);
				iterCEnd = m_ChildList.end();
				continue;
			}

			else if (!(*iterC)->IsEnable())
			{
				++iterC;
				continue;
			}

			(*iterC)->Collision(fDeltaTime);
			++iterC;
		}
	}

	void Bindable::PreDraw(float fDeltaTime)
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd;)
		{
			if (!(*iter)->IsActive())
			{
				iter = m_ChildList.erase(iter);
				iterEnd = m_ChildList.end();
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

	void Bindable::Bind()
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			switch ((*iter)->GetObjectType())
			{
			case Engine::OBJECT_TYPE::BIND:
			case Engine::OBJECT_TYPE::COLLIDER:
				(*iter)->Bind();
				break;
			}
		}
	}

	void Bindable::PostBind()
	{
		std::list<std::shared_ptr<Bindable>>::iterator iter = m_ChildList.begin();
		std::list<std::shared_ptr<Bindable>>::iterator iterEnd = m_ChildList.end();

		for (; iter != iterEnd; ++iter)
		{
			(*iter)->PostBind();
		}
	}

	std::shared_ptr<Bindable> Bindable::Clone()
	{
		return nullptr;
	}

	void Bindable::Reset()
	{
	}

	void Bindable::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_eBindableType, 4, 1, pFile);
	}

	void Bindable::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_eBindableType, 4, 1, pFile);
	}
}
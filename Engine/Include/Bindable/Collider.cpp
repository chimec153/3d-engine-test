#include "Collider.h"
#include "../Collision/CollisionManager.h"

namespace Engine
{
	Collider::Collider() :
		Bindable()
		, m_eColliderType(COLLIDER_TYPE::NONE)
		, m_eChannel(COLLISION_CHANNEL::NORMAL)
	{
		SetObjectType(OBJECT_TYPE::COLLIDER);
	}

	Collider::Collider(const Collider& collider) :
		Bindable(collider)
		, m_eColliderType(collider.m_eColliderType)
		, m_vCross()
		, m_PrevColliderList()
		, m_CallBack()
		, m_eChannel(collider.m_eChannel)
	{
	}

	const COLLIDER_TYPE Collider::GetColliderType() const
	{
		return m_eColliderType;
	}

	void Collider::SetColliderType(COLLIDER_TYPE eType)
	{
		m_eColliderType = eType;
	}

	const Vector3& Collider::GetCross() const
	{
		return m_vCross;
	}

	void Collider::SetCross(const Vector3& vCross)
	{
		m_vCross = vCross;
	}

	void Collider::AddPrevCollider(Collider* pCollider)
	{
		m_PrevColliderList.push_back(pCollider);
	}

	bool Collider::HasPrevCollider(Collider* pCollider) const
	{
		std::list<Collider*>::const_iterator iter = m_PrevColliderList.begin();
		std::list<Collider*>::const_iterator iterEnd = m_PrevColliderList.end();

		for (; iter != iterEnd; ++iter)
		{
			if (*iter == pCollider)
			{
				return true;
			}
		}

		return false;
	}

	void Collider::DeletePrevCollider(Collider* pCollider)
	{
		std::list<Collider*>::const_iterator iter = m_PrevColliderList.begin();
		std::list<Collider*>::const_iterator iterEnd = m_PrevColliderList.end();

		for (; iter != iterEnd; ++iter)
		{
			if (*iter == pCollider)
			{
				m_PrevColliderList.erase(iter);
				return;
			}
		}

	}

	void Collider::SetCallBack(COLLISION_TYPE eType, void(*pFunc)(Collider*, Collider*, float))
	{
		assert(static_cast<int>(eType) >= 0 && eType < COLLISION_TYPE::END);

		m_CallBack[static_cast<int>(eType)] = std::bind(pFunc, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	}

	void Collider::Call(COLLISION_TYPE eType, Collider* pDest, float fDeltaTime)
	{
		if (m_CallBack[static_cast<int>(eType)])
		{
			m_CallBack[static_cast<int>(eType)](this, pDest, fDeltaTime);
		}
	}

	const std::list<class Collider*>& Collider::GetPrevColliderList() const
	{
		return m_PrevColliderList;
	}

	void Collider::ClearCallBack()
	{
		for (int i = 0; i < static_cast<int>(COLLISION_TYPE::END); ++i)
		{
			m_CallBack[i] = nullptr;
		}
	}

	void Collider::SetChannel(COLLISION_CHANNEL eChannel)
	{
		m_eChannel = eChannel;
	}

	COLLISION_CHANNEL Collider::GetChannel() const noexcept
	{
		return m_eChannel;
	}

	void Collider::Collision(float fDeltaTime)
	{
		CollisionManager::GetInst()->AddCollider(this);

		__super::Collision(fDeltaTime);
	}

	bool Collider::Collision(Collider* pDest, float fDeltaTime)
	{
		return false;
	}
	void Collider::Save(FILE* pFile)
	{
		__super::Save(pFile);

		fwrite(&m_eColliderType, 4, 1, pFile);
		fwrite(&m_eChannel, 4, 1, pFile);
	}
	void Collider::Load(FILE* pFile)
	{
		__super::Load(pFile);

		fread(&m_eColliderType, 4, 1, pFile);
		fread(&m_eChannel, 4, 1, pFile);
	}
}
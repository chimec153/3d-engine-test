#include "Collider.h"
#include "../Collision/CollisionManager.h"

namespace Engine
{
	Collider::Collider() :
		Component()
		, m_eColliderType(COLLIDER_TYPE::NONE)
		, m_eChannel(COLLISION_CHANNEL::NORMAL)
	{
		// OBJECT_TYPE was a Bindable concept; on the Component side the
		// "this is a collider" tag lives in m_eColliderType and the
		// COMPONENT_TYPE enum (per-subtype). No SetObjectType call needed.
	}

	Collider::Collider(const Collider& collider) :
		Component(collider)
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
		// Linear scan over a contiguous vector. For the small list sizes
		// typical here (<10 entries) this beats unordered_set::find by a
		// large margin — pointer compare against a hot cache line vs.
		// hash + bucket dance.
		//
		// Raw data pointer + size loop bypasses iterator validation that
		// the Debug STL otherwise wraps every begin()/end()/operator++
		// with (each adding an RTC_CheckStackVars and bounds check). In
		// the O(N²) collision pass this was visible as ~12% of frame time
		// in the function body itself — most of which was Debug iterator
		// overhead, not the actual compares.
		Collider* const* pData = m_PrevColliderList.data();
		const size_t      iSize = m_PrevColliderList.size();
		for (size_t i = 0; i < iSize; ++i)
			if (pData[i] == pCollider) return true;
		return false;
	}

	void Collider::DeletePrevCollider(Collider* pCollider)
	{
		// Swap-and-pop: order in this list isn't observable externally,
		// so an O(1) remove beats erase + shift.
		for (size_t i = 0; i < m_PrevColliderList.size(); ++i)
		{
			if (m_PrevColliderList[i] == pCollider)
			{
				m_PrevColliderList[i] = m_PrevColliderList.back();
				m_PrevColliderList.pop_back();
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

	const std::vector<class Collider*>& Collider::GetPrevColliderList() const
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
#pragma once

#include "../Types.h"

namespace Engine
{
	template <typename Child, typename Parent>
	struct Compare
	{
		static bool check(Parent*);
		static short check(...);

		enum
		{
			value = sizeof(check(static_cast<Child*>(0)) == sizeof(bool))
		};
	};

	template <typename T>
	class ENGINE_DLL CPtr
	{
	public:
		CPtr() :
			m_pOwner(nullptr)
		{

		}

		CPtr(const CPtr<T>& ptr) :
			m_pOwner(ptr.m_pOwner)
		{
			if (m_pOwner)
			{
				m_pOwner->AddRef();
			}
		}

		template <typename P>
		CPtr(const CPtr<P>& ptr) :
			m_pOwner(static_cast<T*>(*ptr))
		{
			if (m_pOwner)
			{
				m_pOwner->AddRef();
			}
		}

		CPtr(T* pOwner) :
			m_pOwner(pOwner)
		{
			if (m_pOwner)
			{
				m_pOwner->AddRef();
			}
		}

		~CPtr()
		{
			SAFE_RELEASE(m_pOwner);
		}

	private:
		T* m_pOwner;

	public:
		bool operator==(T* ptr)	const
		{
			return m_pOwner == ptr;
		}

		bool operator!=(T* ptr)	const
		{
			return m_pOwner != ptr;
		}

		bool operator==(const CPtr<T>& ptr)	const
		{
			return m_pOwner == ptr.m_pOwner;
		}

		bool operator!=(const CPtr<T>& ptr)	const
		{
			return m_pOwner != ptr.m_pOwner;
		}

		template <typename P>
		bool operator==(const CPtr<P>& ptr)	const
		{
			return m_pOwner == ptr;
		}
		inline T* Get()	const
		{
			return m_pOwner;
		}

		inline T* operator*()	const noexcept
		{
			return m_pOwner;
		}

		inline T* operator->()	const noexcept
		{
			return m_pOwner;
		}

		T** operator&()
		{
			SAFE_RELEASE(m_pOwner);
			return &m_pOwner;
		}

		void operator=(const CPtr<T>& ptr)
		{
			Set(ptr.m_pOwner);
		}

		void operator=(T* pOwner)
		{
			Set(pOwner);
		}

		operator bool()	const
		{
			return m_pOwner;
		}

		template <typename Parent>
		bool IsParent(Parent* pParent)	const
		{
			return Compare<T, Parent>::value;
		}

		template <typename Child>
		bool IsChild(Child* pChild)	const
		{
			return Compare<Child, T>::value;
		}

		T** GetAdressof()
		{
			return &m_pOwner;
		}

		template <typename P>
		operator CPtr<P>()
		{
			return static_cast<P*>(m_pOwner);
		}

		template <typename P>
		CPtr<P> Change()	const
		{
			return static_cast<P*>(m_pOwner);
		}

	private:
		void Set(T* pOwner)
		{
			SAFE_RELEASE(m_pOwner);

			m_pOwner = pOwner;

			if (m_pOwner)
			{
				m_pOwner->AddRef();
			}
		}
	};

	template <typename T, typename P>
	bool operator==(T* pSrc, const CPtr<P>& pDest)
	{
		return pSrc == *pDest;
	}
}
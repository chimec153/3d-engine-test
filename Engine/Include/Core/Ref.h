#pragma once

#include "../Types.h"

namespace Engine
{
	class ENGINE_DLL CRef : public std::enable_shared_from_this<CRef>
	{
	public:
		CRef() :
			m_iRef(0)
			, m_bActive(true)
			, m_bEnable(true)
			, m_strTag()
		{

		}

		CRef(const CRef& ref) :
			m_iRef(0)
			, m_bActive(ref.m_bActive)
			, m_bEnable(ref.m_bEnable)
			, m_strTag(ref.m_strTag)
		{

		}

		virtual ~CRef()
		{

		}

	private:
		int m_iRef;
		bool m_bActive;
		bool m_bEnable;
		std::string m_strTag;

	public:
		int Release()
		{
			if (--m_iRef == 0)
			{
				delete this;
				return 0;
			}

			return m_iRef;
		}

		void AddRef()
		{
			++m_iRef;
		}

		void SetTag(const std::string& strTag)
		{
			m_strTag = strTag;
		}

		const std::string& GetTag()	const
		{
			return m_strTag;
		}

		bool IsActive()	const
		{
			return m_bActive;
		}

		void InActivate()
		{
			m_bActive = false;
		}

		bool IsEnable()	const
		{
			return m_bEnable;
		}

		void Enable()
		{
			m_bEnable = true;
		}

		void Disable()
		{
			m_bEnable = false;
		}
	public:
		virtual void SaveFromFullPath(const char* pFullPath);
		virtual void SaveFromPath(const char* pFilePath, const std::string& strPathKey = ROOT_PATH);
		virtual void LoadFromPath(const char* pFilePath, const std::string& strPathKey = ROOT_PATH);
		virtual void Save(FILE* pFile);
		virtual void Load(FILE* pFile);

	};

}
#pragma once

#include "../Core/Ptr.h"
#include "../Core/Window.h"
#include "Sampler.h"
#include "RasterizerState.h"
#include "Material.h"
#include "FbxLoader.h"

namespace Engine
{
	template <typename T>
	class ENGINE_DLL BindableManager
	{
	private:
		BindableManager();
		~BindableManager();

	private:
		static BindableManager<T>* m_pInst;

	public:
		static BindableManager<T>* GetInst()
		{
			if (!m_pInst)
			{
				m_pInst = dbg_new BindableManager<T>;
			}

			return m_pInst;
		}
		static void DestroyInst()
		{
			if (m_pInst)
			{
				delete m_pInst;
				m_pInst = nullptr;
			}
		}

	private:
		std::unordered_map<std::string, class std::shared_ptr<T>>	m_mapBindable;

	public:
		template <typename ...Args>
		std::shared_ptr<T> CreateBindable(const std::string& strTag, Args... args)
		{
			std::shared_ptr<T> pBindable = FindBindable(strTag);

			if (pBindable != nullptr)
			{
				return nullptr;
			}

			pBindable = std::make_shared<T>(args...);

			pBindable->SetTag(strTag);

			m_mapBindable.insert(std::make_pair(strTag, pBindable));

			return pBindable;
		}

		std::shared_ptr<T> FindBindable(const std::string& strTag)	const
		{
			typename::std::unordered_map<std::string, class std::shared_ptr<T>>::const_iterator iter = m_mapBindable.find(strTag);

			if (iter == m_mapBindable.end())
			{
				return nullptr;
			}

			return iter->second;
		}

	public:
		bool Init()
		{
			return false;
		}
	};


	template <typename T>
	ENGINE_DLL std::shared_ptr<T> StaticFindBindable(const std::string& strTag);

	template <typename T, typename ...Args>
	ENGINE_DLL std::shared_ptr<T> StaticCreateBindable(const std::string& strTag, Args... args);

	template <typename T, typename ...Args>
	std::shared_ptr<T> StaticCreateBindable(const std::string& strTag, Args... args)
	{
		return BindableManager<T>::GetInst()->BindableManager<T>::CreateBindable<Args...>(strTag, args...);
	}
}
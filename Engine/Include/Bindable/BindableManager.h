#pragma once

#include "../Core/Ptr.h"
#include "../Core/Window.h"
#include "Sampler.h"
#include "RasterizerState.h"
#include "Material.h"
#include "FbxLoader.h"
#include "BindableRegistry.h"

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
				// Register this concrete type's destroyer so a single
				// BindableRegistry::DestroyAll() at shutdown releases
				// every cached Bindable (and the D3D resources they own)
				// before the Graphics device goes away.
				BindableRegistry::Register([]() { BindableManager<T>::DestroyInst(); });
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

			AddBindable(pBindable);

			return pBindable;
		}

		void AddBindable(std::shared_ptr<T> pBindable)
		{
			m_mapBindable.insert(std::make_pair(pBindable->GetTag(), pBindable));
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

		// Enumerate every registered bindable — used by editor asset
		// browsers (e.g. material picker dropdown listing all loaded .mat
		// assets).
		const std::unordered_map<std::string, std::shared_ptr<T>>& GetMap() const
		{
			return m_mapBindable;
		}

		// Drops the tag→bindable index. Outstanding shared_ptrs (held by
		// e.g. a live Scene) keep the objects alive; only future
		// FindBindable lookups stop seeing them. Used by ProjectModule when
		// swapping to a different game project's content.
		void Clear()
		{
			m_mapBindable.clear();
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
#pragma once
#include "../Core/Ptr.h"
#include "../Core/Ref.h"
namespace Engine
{
	class ENGINE_DLL Bindable :
		public CRef
	{
		friend class Graphics;
	protected:
		Bindable();
		Bindable(const Bindable& bindable);
		virtual ~Bindable() noexcept;

	private:
		BINDABLE_TYPE m_eBindableType;
		OBJECT_TYPE m_eObjectType;
		class Bindable* m_pParent;
		std::list<std::shared_ptr<class Bindable>> m_ChildList;
		class Scene* m_pScene;
		class Layer* m_pLayer;

	public:
		void SetBindableType(BINDABLE_TYPE eType);
		BINDABLE_TYPE GetBindableType() const;
		class Bindable* GetParent() const;
		void SetParent(class Bindable* pParent);
		const std::list<std::shared_ptr<Bindable>>& GetChildList()	const;
		virtual void AddChild(const std::shared_ptr<class Bindable>& pChild);
		void SetObjectType(OBJECT_TYPE eType);
		OBJECT_TYPE GetObjectType() const;
		void SetScene(class Scene* pScene);
		void SetLayer(class Layer* pLayer);
		class Scene* GetScene()	const;
		Layer* GetLayer()	const;

		template <typename T>
		void FindChilds(std::vector<std::shared_ptr<T>>& vecBindable)   const
		{
			std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
			std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

			for (; iter != iterEnd; ++iter)
			{
				if (typeid(T) == typeid(*(*iter).get()))
				{
					vecBindable.push_back(std::static_pointer_cast<T>(*iter));
					continue;
				}

				(*iter)->FindChilds<T>(vecBindable);
			}
		}

		std::shared_ptr<Bindable> FindChild(BINDABLE_TYPE eType)   const;
		void FindChilds(BINDABLE_TYPE eType, std::vector<std::shared_ptr<Bindable>>& vecBindables)   const;
		std::shared_ptr<Bindable> FindChild(const std::string& strTag) const;
		void DeleteChild(std::shared_ptr<Bindable> pBindable);
		std::shared_ptr<Bindable> FindChild(OBJECT_TYPE eType)  const;
		template <typename T>
		std::shared_ptr<T> FindChild() const
		{
			std::list<std::shared_ptr<Bindable>>::const_iterator iter = m_ChildList.begin();
			std::list<std::shared_ptr<Bindable>>::const_iterator iterEnd = m_ChildList.end();

			for (; iter != iterEnd; ++iter)
			{
				if (typeid(*(*iter).get()) == typeid(T))
				{
					return std::static_pointer_cast<T>(*iter);
				}

				std::shared_ptr<T> pChild = (*iter)->FindChild<T>();

				if (pChild)
				{
					return pChild;
				}
			}

			return nullptr;
		}

		template <typename T>
		std::shared_ptr<T> FindAndAddBind(const std::string& strTag)
		{
			std::shared_ptr<T> pBindable = StaticFindBindable<T>(strTag);

			if (pBindable == nullptr)
			{
				assert(false);
				return nullptr;
			}

			AddChild(pBindable);

			return pBindable;
		}
	public:
		template <typename T, typename ...Args>
		std::shared_ptr<T> CreateBindable(const std::string& strTag, Args... args)
		{
			std::shared_ptr<T> pBindable = std::make_shared<T>(args...);

			if (!pBindable)
			{
				return nullptr;
			}

			pBindable->SetTag(strTag);

			if (!pBindable->Init())
			{
				return nullptr;
			}

			AddChild(pBindable);

			return pBindable;
		}
	public:
		virtual bool Init();
		virtual void Start();
		virtual void Input(float fDeltaTime);
		virtual void Update(float fDeltaTime);
		virtual void FixedUpdate(float fDeltaTime);
		virtual void Collision(float fDeltaTime);
		virtual void PostUpdate(float fDeltaTime);
		virtual void PreDraw(float fDeltaTime);
		virtual void Bind();
		virtual void PostBind();
		virtual std::shared_ptr<Bindable> Clone() = 0;
		virtual void Reset();

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;

	public:
		static std::shared_ptr<Bindable> CreateBindable(BINDABLE_TYPE eType);
		static std::shared_ptr<Bindable> FindBindable(BINDABLE_TYPE eType, const std::string& strBind);
	};

}
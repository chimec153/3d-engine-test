#pragma once
#include "../Core/Ref.h"
#include "../Component/Component.h"

namespace Engine
{
	// Phase E1 — GameObject is the entity / "Actor" in the scene.
	// Sibling of Bindable and Component under CRef:
	//
	//   CRef
	//   ├── Bindable    (GPU resource)
	//   ├── Component   (behavior/aspect attached to a GameObject)
	//   └── GameObject  (entity, container of Components)
	//
	// Inheriting from Component would conflate "entity" with "behavior" —
	// Unity/Unreal both keep these as separate hierarchies and we follow
	// suit. GameObject's lifecycle dispatches to its Components' lifecycle.
	class ENGINE_DLL GameObject : public CRef
	{
	public:
		GameObject();
		GameObject(const GameObject& other);
		virtual ~GameObject() override;

	private:
		std::list<std::shared_ptr<Component>> m_Components;

		// Optional entity-level scene graph (parent/child entities).
		// Spatial parent/child is on Transform; this is for organizational
		// hierarchy (e.g., enemies-folder > Goblin1, Goblin2 ...).
		GameObject* m_pParent;
		std::list<GameObject*> m_Children;

		// Phase E5 — Layer back-pointer. Set by Scene::CreateGameObject
		// before Init runs and by Layer::AddGameObject (idempotently). Lets
		// game-class GameObjects reach Layer/Scene the way Drawable
		// subclasses used to via Drawable::GetScene().
		class Layer* m_pLayer;

	public:
		// Construct a Component subclass T, add it to this GameObject, and
		// return the typed pointer. The Component's Init() runs after
		// attachment so it can query siblings via GetComponent<U>().
		template <typename T, typename ...Args>
		std::shared_ptr<T> AddComponent(const std::string& strTag, Args... args)
		{
			auto pComp = std::make_shared<T>(args...);
			pComp->SetTag(strTag);
			pComp->SetGameObjectOwner(this);
			m_Components.push_back(std::static_pointer_cast<Component>(pComp));
			if (!pComp->Init())
				return nullptr;
			return pComp;
		}

		// Add a pre-constructed Component (used when component is created
		// elsewhere or shared across owners — rare).
		void AddComponent(const std::shared_ptr<Component>& pComp);

		// Find the first Component matching type T. Linear search; expected
		// to be called sparingly (typically cached at Init time).
		template <typename T>
		std::shared_ptr<T> GetComponent() const
		{
			for (const auto& p : m_Components)
			{
				if (auto cast = std::dynamic_pointer_cast<T>(p))
					return cast;
			}
			return nullptr;
		}

		template <typename T>
		std::vector<std::shared_ptr<T>> GetComponents() const
		{
			std::vector<std::shared_ptr<T>> out;
			for (const auto& p : m_Components)
			{
				if (auto cast = std::dynamic_pointer_cast<T>(p))
					out.push_back(cast);
			}
			return out;
		}

		std::shared_ptr<Component> FindComponent(COMPONENT_TYPE eType) const;
		std::shared_ptr<Component> FindComponent(const std::string& strTag) const;
		const std::list<std::shared_ptr<Component>>& GetComponentList() const;

		// Entity-level scene graph (separate from Transform parenting).
		GameObject* GetParent() const;
		void SetParent(GameObject* pParent);
		const std::list<GameObject*>& GetChildren() const;

		// Phase E5 — Layer / Scene access for game-class code that used to
		// reach the Scene via Drawable::GetScene().
		class Layer* GetLayer() const { return m_pLayer; }
		void SetLayer(class Layer* pLayer) { m_pLayer = pLayer; }
		class Scene* GetScene() const;

	public:
		virtual bool Init();
		virtual void Start();
		virtual void Input(float fDeltaTime);
		virtual void Update(float fDeltaTime);
		virtual void FixedUpdate(float fDeltaTime);
		virtual void Collision(float fDeltaTime);
		virtual void PostUpdate(float fDeltaTime);
		virtual void PreDraw(float fDeltaTime);

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;
	};
}

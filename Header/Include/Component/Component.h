#pragma once
#include "../Core/Ref.h"

namespace Engine
{
	// Sibling of Bindable. Bindable is for GPU-state (Bind/PostBind);
	// Component is for CPU-side scene/game logic with a per-frame lifecycle.
	// Both inherit CRef. Once migrations are complete, Bindable's m_ChildList
	// holds GPU resources only and Component's m_ChildList holds non-GPU
	// scene logic.
	enum class COMPONENT_TYPE
	{
		NONE,
		TRANSFORM,
		CAMERA,
		COLLIDER_LINE,
		COLLIDER_SPHERE,
		COLLIDER_MESH,
		COLLIDER_OBB,
		ANIMATION,
		AGENT,
		NAV_MESH,
		SOUND,
		MOUSE,
		JOINT_SOCKET,
		LIGHT,
		PAPERBURN,
		END
	};

	class ENGINE_DLL Component : public CRef
	{
	protected:
		Component();
		Component(const Component& comp);
		virtual ~Component() noexcept;

	private:
		COMPONENT_TYPE m_eComponentType;
		Component* m_pParent;
		std::list<std::shared_ptr<Component>> m_ChildList;

		// Phase E5 — Drawable owner field removed (no live Drawable
		// hosts remain). GameObject is the only host type now.
		class GameObject* m_pGameObjectOwner;

	public:
		void SetComponentType(COMPONENT_TYPE eType);
		COMPONENT_TYPE GetComponentType() const;
		Component* GetParent() const;
		void SetParent(Component* pParent);
		// Phase E5 — Drawable GetOwner/SetOwner removed.
		class GameObject* GetGameObjectOwner() const;
		void SetGameObjectOwner(class GameObject* pOwner);

		// Phase E5 — host-agnostic Transform accessor. Resolves the
		// Drawable owner's Transform first, falls back to the GameObject
		// owner's Transform Component. Lets Components (colliders, sound,
		// animation, etc.) reach the host transform regardless of whether
		// they're attached to a Drawable or a GameObject.
		std::shared_ptr<class Transform> GetHostTransform() const;
		const std::list<std::shared_ptr<Component>>& GetChildList() const;
		virtual void AddChild(const std::shared_ptr<Component>& pChild);
		std::shared_ptr<Component> FindChild(COMPONENT_TYPE eType) const;
		std::shared_ptr<Component> FindChild(const std::string& strTag) const;
		void DeleteChild(std::shared_ptr<Component> pComp);

		template <typename T>
		std::shared_ptr<T> FindChild() const
		{
			for (const auto& pChild : m_ChildList)
			{
				if (typeid(*pChild.get()) == typeid(T))
				{
					return std::static_pointer_cast<T>(pChild);
				}

				std::shared_ptr<T> p = pChild->FindChild<T>();
				if (p)
					return p;
			}
			return nullptr;
		}

		template <typename T>
		void FindChilds(std::vector<std::shared_ptr<T>>& vec) const
		{
			for (const auto& pChild : m_ChildList)
			{
				if (typeid(T) == typeid(*pChild.get()))
				{
					vec.push_back(std::static_pointer_cast<T>(pChild));
					continue;
				}
				pChild->FindChilds<T>(vec);
			}
		}

		// Create-and-add helper: builds a Component subclass T, tags it,
		// Init's it, and attaches to this Component's child list.
		template <typename T, typename ...Args>
		std::shared_ptr<T> CreateComponent(const std::string& strTag, Args... args)
		{
			std::shared_ptr<T> pComp = std::make_shared<T>(args...);
			if (!pComp) return nullptr;
			pComp->SetTag(strTag);
			if (!pComp->Init())
				return nullptr;
			AddChild(std::static_pointer_cast<Component>(pComp));
			return pComp;
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
		virtual std::shared_ptr<Component> Clone() = 0;
		virtual void Reset();

		// Components that own a Transform (e.g., Camera, future Light)
		// override this so the parent Drawable can wire its m_pTransform
		// hierarchy up at AddChild time. Default returns nullptr (most
		// Components don't have an independent transform).
		virtual std::shared_ptr<class Transform> GetTransform() const { return nullptr; }

		// Phase E5 — Render-time GPU bind hook for "decorator" Components
		// (e.g., PaperBurn) that piggyback on a sibling MeshRenderer's draw.
		// Invoked by MeshRendererComponent::Bind on every sibling Component
		// of the owning GameObject right before the mesh draw. Default is
		// no-op; only a few Components (PaperBurn, future shader-effect
		// components) override.
		virtual void RenderBind() {}

		// Symmetric counterpart to RenderBind — invoked by
		// MeshRendererComponent::PostBind / Drawable::PostBind after the
		// mesh draw so a Component can release SRV slots / unbind state
		// it set up in RenderBind. Default no-op.
		virtual void RenderUnbind() {}

	public:
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;
	};
}

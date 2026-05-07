#pragma once

#include "../Core/Ref.h"
// Phase E7 — FindComponent(COMPONENT_TYPE) declared below references the
// COMPONENT_TYPE enum, which lives in Component.h. Engine + Client builds
// happen to include Component.h before Layer.h transitively, but consumers
// like Editor's unity build can hit Layer.h first and fail to parse.
#include "../Component/Component.h"

template <typename T>
class std::shared_ptr;
namespace Engine
{
    class ENGINE_DLL Layer :
        public CRef
    {
        friend class Scene;

    public:
        Layer();
    public:
        virtual ~Layer() override;

    private:
        int     m_iZOrder;
        // Phase B.5 — top-level Components (Camera, Light controllers, etc.).
        // Layer drives their lifecycle alongside m_GameObjectList.
        std::list<std::shared_ptr<class Component>> m_ComponentList;
        // Phase E1 — GameObjects (entity / Actor) that are scene-resident.
        // Their MeshRendererComponent self-registers with RenderManager
        // each PreDraw, so the Layer doesn't need a parallel render list.
        std::list<std::shared_ptr<class GameObject>> m_GameObjectList;
        class Scene* m_pScene;
        std::shared_ptr<class LoadingThread> m_pLoadingThread;

    public:
        void SetZOrder(int iZOrder);
        int GetZOrder() const;
        void AddComponent(const std::shared_ptr<class Component>& pComp);
        void AddGameObject(const std::shared_ptr<class GameObject>& pObj);
        const std::list<std::shared_ptr<class GameObject>>& GetGameObjectList() const;
        std::shared_ptr<class GameObject> FindGameObject(const std::string& strTag) const;
        void DeleteGameObject(std::shared_ptr<class GameObject> pObj);
        void SetScene(class Scene* pScene);
        class Scene* GetScene() const { return m_pScene; }
        const std::list<std::shared_ptr<class Component>>& GetComponentList() const;
        const std::shared_ptr<class LoadingThread>& GetLoadingThread()  const;
        std::shared_ptr<class Component> FindComponent(const std::string& strTag) const;
        std::shared_ptr<class Component> FindComponent(COMPONENT_TYPE eType) const;
        void DeleteComponent(std::shared_ptr<class Component> pComp);

    public:
        void Input(float fDeltaTime);
        void Update(float fDeltaTime);
        void FixedUpdate(float fDeltaTime);
        void Collision(float fDeltaTime);
        void PostUpdate(float fDeltaTime);
        void PreDraw(float fDeltaTime);
        void Draw();
        void CreateLoadingThread(const TCHAR* pFullPath);

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
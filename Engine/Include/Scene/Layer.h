#pragma once

#include "../Core/Ref.h"

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
        std::list<class std::shared_ptr<class Bindable>>   m_DrawList;
        // Phase B.5 — Component-side companion to m_DrawList. Holds
        // top-level Components (Camera, Light controllers, etc.) that
        // shouldn't be Drawables. Lifecycle iteration runs over both
        // lists; rendering only touches m_DrawList.
        std::list<std::shared_ptr<class Component>> m_ComponentList;
        // Phase E1 — GameObjects (entity / Actor) that are scene-resident.
        // Lifecycle iteration includes these; rendering side will pick up
        // their MeshRenderer Components in a later phase (E2). For now
        // GameObjects can exist and run their components but won't render.
        std::list<std::shared_ptr<class GameObject>> m_GameObjectList;
        class Scene* m_pScene;
        std::shared_ptr<class LoadingThread> m_pLoadingThread;

    public:
        void SetZOrder(int iZOrder);
        int GetZOrder() const;
        void AddDrawable(const class std::shared_ptr<Bindable>& pDrawable);
        void AddComponent(const std::shared_ptr<class Component>& pComp);
        void AddGameObject(const std::shared_ptr<class GameObject>& pObj);
        const std::list<std::shared_ptr<class GameObject>>& GetGameObjectList() const;
        std::shared_ptr<class GameObject> FindGameObject(const std::string& strTag) const;
        void DeleteGameObject(std::shared_ptr<class GameObject> pObj);
        void SetScene(class Scene* pScene);
        const std::list<class std::shared_ptr<class Bindable>>& GetDrawList()   const;
        const std::list<std::shared_ptr<class Component>>& GetComponentList() const;
        const std::shared_ptr<class LoadingThread>& GetLoadingThread()  const;
        std::shared_ptr<Bindable> FindDrawable(const std::string& strTag)    const;
        std::shared_ptr<Bindable> FindDrawable(BINDABLE_TYPE eType)    const;
        std::shared_ptr<class Component> FindComponent(const std::string& strTag) const;
        std::shared_ptr<class Component> FindComponent(COMPONENT_TYPE eType) const;
        void DeleteDrawable(std::shared_ptr<Bindable> pDrawable);
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
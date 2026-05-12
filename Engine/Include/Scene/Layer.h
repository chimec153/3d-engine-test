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
        // GameObjects (entity / Actor) that are scene-resident. Top-level
        // Components like Camera / Light / Mouse are now hosted on wrapper
        // GameObjects too — there is no parallel Component list anymore.
        // MeshRendererComponent self-registers with RenderManager each
        // PreDraw, so the Layer also doesn't need a parallel render list.
        std::list<std::shared_ptr<class GameObject>> m_GameObjectList;
        class Scene* m_pScene;
        std::shared_ptr<class LoadingThread> m_pLoadingThread;

    public:
        void SetZOrder(int iZOrder);
        int GetZOrder() const;
        void AddGameObject(const std::shared_ptr<class GameObject>& pObj);
        const std::list<std::shared_ptr<class GameObject>>& GetGameObjectList() const;
        std::shared_ptr<class GameObject> FindGameObject(const std::string& strTag) const;
        void DeleteGameObject(std::shared_ptr<class GameObject> pObj);
        void SetScene(class Scene* pScene);
        class Scene* GetScene() const { return m_pScene; }
        const std::shared_ptr<class LoadingThread>& GetLoadingThread()  const;

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
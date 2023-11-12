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
        class Scene* m_pScene;
        std::shared_ptr<class LoadingThread> m_pLoadingThread;

    public:
        void SetZOrder(int iZOrder);
        int GetZOrder() const;
        void AddDrawable(const class std::shared_ptr<Bindable>& pDrawable);
        void SetScene(class Scene* pScene);
        const std::list<class std::shared_ptr<class Bindable>>& GetDrawList()   const;
        const std::shared_ptr<class LoadingThread>& GetLoadingThread()  const;
        std::shared_ptr<Bindable> FindDrawable(const std::string& strTag)    const;

    public:
        void Input(float fDeltaTime);
        void Update(float fDeltaTime);
        void FixedUpdate(float fDeltaTime);
        void Collision(float fDeltaTime);
        void PreDraw(float fDeltaTime);
        void Draw();
        void DrawListImgui();
        void CreateLoadingThread(const TCHAR* pFullPath);
    };

}
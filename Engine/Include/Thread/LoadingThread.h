#pragma once
#include "Thread.h"
namespace Engine
{
    class GameObject;

    // Phase E7 — LoadingThread now produces a GameObject populated with a
    // Transform + MeshRendererComponent (the MR is filled with the loaded
    // mesh / material / textures + default shaders and pipeline state via
    // MeshLoader::LoadInto). Replaces the previous Drawable-typed result.
    class ENGINE_DLL LoadingThread :
        public Thread
    {
    public:
        LoadingThread();
        virtual ~LoadingThread() override;

    private:
        TCHAR m_strFullPath[MAX_PATH];
        std::shared_ptr<GameObject> m_pGameObject;

    public:
        const std::shared_ptr<GameObject>& GetGameObject() const;
        void SetFullPath(const TCHAR* pFullPath);

    public:
        virtual void Run() override;
    };

}

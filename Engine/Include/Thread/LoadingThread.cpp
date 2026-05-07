#include "LoadingThread.h"
#include "../Bindable/MeshLoader.h"
#include "../Bindable/Transform.h"
#include "../Component/MeshRendererComponent.h"
#include "../GameObject/GameObject.h"

namespace Engine
{
    LoadingThread::LoadingThread() :
        m_strFullPath()
        , m_pGameObject(nullptr)
    {
    }

    LoadingThread::~LoadingThread()
    {
    }

    const std::shared_ptr<GameObject>& LoadingThread::GetGameObject() const
    {
        return m_pGameObject;
    }

    void LoadingThread::SetFullPath(const TCHAR* pFullPath)
    {
        _tcscpy_s(m_strFullPath, pFullPath);
    }

    void LoadingThread::Run()
    {
        // Build the entity skeleton on this worker thread, then drive the
        // MeshLoader into the entity's MeshRendererComponent. The actual
        // file parse + GPU resource creation runs here so the main thread
        // only pays for AddGameObject when IsFinish() flips.
        m_pGameObject = std::make_shared<GameObject>();
        if (!m_pGameObject->Init()) return;

        m_pGameObject->AddComponent<Transform>("transform");
        std::shared_ptr<MeshRendererComponent> pMR =
            m_pGameObject->AddComponent<MeshRendererComponent>("mesh_renderer");

        if (pMR) MeshLoader::LoadInto(m_strFullPath, "", pMR);
    }
}

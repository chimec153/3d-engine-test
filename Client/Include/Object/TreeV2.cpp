#include "TreeV2.h"

namespace Client
{
    TreeV2::TreeV2() = default;

    bool TreeV2::Init(ID3D11Device* device, const wchar_t* assetPath)
    {
        m_mesh = std::make_shared<Engine::RenderV2::Drawables::Mesh>();
        return m_mesh->Init(device, assetPath);
    }

    void TreeV2::Update(float dt)
    {
        if (m_mesh) m_mesh->Update(dt);
    }

    void TreeV2::Submit(Engine::RenderV2::RenderQueue& queue,
                       const Engine::RenderV2::FrameInfo& frame)
    {
        if (m_mesh) m_mesh->Submit(queue, frame);
    }

    void TreeV2::SetPosition(const DirectX::XMFLOAT3& pos)
    {
        if (m_mesh) m_mesh->SetPosition(pos);
    }
}

#pragma once

#include "RenderV2/Drawable.h"
#include "RenderV2/Drawables/Mesh.h"
#include <memory>

namespace Client
{
    // First Client object on the V2 path. Mirrors the legacy `Tree` (just
    // loads SmallCampingBundle/Tree/Tree.obj); behaviour-wise nothing fancy.
    // Class structure follows Phase 5 pattern: a Client-owned game object
    // that *is* a RenderV2::Drawable via composition with a Mesh.
    class TreeV2 : public Engine::RenderV2::Drawable
    {
    public:
        TreeV2();
        ~TreeV2() override = default;

        // Builds the underlying mesh once. `assetPath` is resolved under
        // MESH_PATH (e.g., L"SmallCampingBundle\\Tree\\Tree.obj"), so the
        // caller can reuse this Tree class for any static prop without
        // editing this file.
        bool Init(struct ID3D11Device* device, const wchar_t* assetPath);

        void Update(float dt) override;
        void Submit(Engine::RenderV2::RenderQueue& queue,
                    const Engine::RenderV2::FrameInfo& frame) override;

        // Pass-through for placement; future Tree-specific game state can
        // hang off this class without growing the Mesh API.
        void SetPosition(const DirectX::XMFLOAT3& pos);

    private:
        std::shared_ptr<Engine::RenderV2::Drawables::Mesh> m_mesh;
    };
}

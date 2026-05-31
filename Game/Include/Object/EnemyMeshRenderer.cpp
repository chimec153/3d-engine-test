#include "EnemyMeshRenderer.h"
#include "Bindable/Material.h"
#include "Bindable/BindableManager.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include <cstring>

namespace Client
{
    void EnemyMeshRenderer::GetInstData(char* pData, int iSize) const
    {
        // Engine base writes transform (192B) + material (44B) = 236B.
        MeshRendererComponent::GetInstData(pData, iSize);

        // Append hit-flash colour, then dissolve time, when the stride has
        // room. The colour pass uses our 256B layout (both written); the
        // shadow inst path passes the engine's larger stride and reads only
        // the transform block, so these trailing bytes are harmless there.
        if (iSize < 236 + 16) return;
        std::shared_ptr<Engine::Material> pMaterial = GetEffectiveMaterial(0, 0);
        const Engine::Vector4 vHitFlash = pMaterial
            ? pMaterial->GetMaterial().vHitFlash
            : Engine::Vector4(0.f, 0.f, 0.f, 0.f);
        std::memcpy(pData + 236, &vHitFlash, 16);

        if (iSize < 236 + 16 + 4) return;
        std::memcpy(pData + 252, &m_fDissolveTime, 4);

        if (iSize < 236 + 16 + 4 + 4) return;
        std::memcpy(pData + 256, &m_fBurnRim, 4);
    }

    std::shared_ptr<Engine::Component> EnemyMeshRenderer::Clone()
    {
        return std::make_shared<EnemyMeshRenderer>(*this);
    }

    void EnemyMeshRenderer::RegisterShaders()
    {
        using namespace Engine;

        // Idempotent — bail if already registered.
        if (StaticFindBindable<VertexShader>(kVSTag)) return;

        // Solo path shaders (lone enemy / no Inst variant). The Inst lookups
        // in RenderManager are derived by appending "Inst" to these tags.
        StaticCreateBindable<VertexShader>(kVSTag, L"EnemyInst.hlsl", "EnemyVS");
        StaticCreateBindable<PixelShader> (kPSTag, L"EnemyInst.hlsl", "EnemyPS");

        // Instanced path shaders.
        std::shared_ptr<VertexShader> pVSInst =
            StaticCreateBindable<VertexShader>(kVSInstTag, L"EnemyInst.hlsl", "EnemyVSInst");
        StaticCreateBindable<PixelShader>(kPSInstTag, L"EnemyInst.hlsl", "EnemyPSInst");

        // Instance input layout. Slot 0 mirrors the engine "Standard" per-vertex
        // layout (so the enemy mesh VB feeds it unchanged); slot 1 is the
        // per-instance block: transform (World/View/LIGHTVP) + material
        // (Material0..3) at the same offsets the base GetInstData writes,
        // followed by the two game attributes HitFlash + PaperTime.
        D3D11_INPUT_ELEMENT_DESC desc[] =
        {
            // --- per-vertex (slot 0) ---
            {"Tangent",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            {"Position",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            {"Normal",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            {"BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            {"Texcoord",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
            // --- per-instance (slot 1) ---
            {"World",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"World",     1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"World",     2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"World",     3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"View",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"View",      1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"View",      2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"View",      3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"LIGHTVP",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"LIGHTVP",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"LIGHTVP",   2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"LIGHTVP",   3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"Material",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"Material",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"Material",  2, DXGI_FORMAT_R32G32_FLOAT,       1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"Material",  3, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"HitFlash",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"PaperTime", 0, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"BurnRim",   0, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        };

        std::shared_ptr<InputLayout> pIL = StaticCreateBindable<InputLayout>(
            "ClientEnemyInstIL", pVSInst, desc,
            static_cast<int>(sizeof(desc) / sizeof(desc[0])), kInstSize);

        if (pVSInst && pIL)
            pVSInst->SetInstInputLayout(pIL);
    }
}

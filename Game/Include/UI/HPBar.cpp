#include "HPBar.h"
#include "../Object/Player.h"
#include "Bindable/Transform.h"
#include "Bindable/UIRenderer.h"
#include "Bindable/Mesh.h"
#include "Bindable/Texture.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Topology.h"
#include "Bindable/BindableManager.h"
#include "Types.h"

namespace Client
{
    // Named (non-anonymous) namespace so Unity / jumbo builds that fold
    // multiple .cpp files into one TU don't collide on common short
    // constant names (EnemyCountHUD.cpp uses the same kBaseX/kBaseY
    // identifiers in its own anonymous namespace, which would otherwise
    // redefine these once both files share a single anonymous scope).
    namespace HPBar_detail
    {
        // NDC: x∈[-1,1], y∈[-1,1] (y=-1 bottom). Bar lives in the
        // bottom-left.
        constexpr float kBaseX  = -0.95f;
        constexpr float kBaseY  = -0.95f;
        constexpr float kHeight =  0.05f;
        constexpr float kFullW  =  0.4f;

        // 1x1 RGBA texture helpers — cached in BindableManager so every
        // HPBar instance shares the same two SRVs.
        std::shared_ptr<Engine::Texture> EnsureBGTexture()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("HPBarBG")) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>("HPBarBG");
            if (!pNew) return nullptr;
            const uint32_t grey = 0xFF303030;
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &grey;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        std::shared_ptr<Engine::Texture> EnsureFillTexture()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("HPBarFill")) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>("HPBarFill");
            if (!pNew) return nullptr;
            const uint32_t red = 0xFF2030E0;   // ABGR memory layout
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &red;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }
    }

    HPBar::HPBar()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool HPBar::Init()
    {
        // No `using namespace HPBar_detail` — EnemyCountHUD.cpp keeps
        // its own kBaseY (etc.) in an anonymous namespace which the
        // Unity build merges into the same TU; the using-directive
        // would then make both kBaseY's visible from Client scope and
        // every reference would become ambiguous. Qualify explicitly.
        if (!Engine::UIControl::Init()) return false;

        auto pVS       = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
        auto pPS       = Engine::StaticFindBindable<Engine::PixelShader> ("UIPS");
        auto pTopology = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
        auto pMesh     = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
        auto pTexBG    = HPBar_detail::EnsureBGTexture();
        auto pTexFill  = HPBar_detail::EnsureFillTexture();
        if (!pVS || !pPS || !pTopology || !pMesh || !pTexBG || !pTexFill) return false;

        // Helper — builds one quad's Transform + UIRenderer pair. Both
        // child Components live on this HPBar's m_ChildList so the
        // Component base ticks their lifecycles (Transform::PostUpdate,
        // UIRenderer::PreDraw self-register). The full-width BG sets
        // its scale once; the fill's scale is rewritten each Update by
        // the player HP ratio.
        auto makeQuad = [&](const char* strTag, float fWidth,
                            const std::shared_ptr<Engine::Texture>& pTex,
                            std::shared_ptr<Engine::Transform>&  pTrOut,
                            std::shared_ptr<Engine::UIRenderer>& pRdOut)
        {
            std::string tagTransform = std::string("transform_") + strTag;
            std::string tagRenderer  = std::string("renderer_")  + strTag;

            pTrOut = CreateComponent<Engine::Transform>(tagTransform);
            if (pTrOut)
            {
                // UI camera type so Transform::PostUpdate's NDC fallback
                // collapses matWorldViewProject to matWorld (no camera
                // multiplication). Scale × Translation lands directly in
                // clip space — matches the UI VS expectation.
                pTrOut->SetCameraType(Engine::CAMERA_TYPE::UI);
                pTrOut->SetScale   (fWidth,                  HPBar_detail::kHeight, 1.f);
                pTrOut->SetPosition(HPBar_detail::kBaseX,    HPBar_detail::kBaseY,  0.f);
            }

            pRdOut = CreateComponent<Engine::UIRenderer>(tagRenderer);
            if (pRdOut)
            {
                pRdOut->SetTarget(pTrOut, pMesh, nullptr);
                pRdOut->SetVertexShader(pVS);
                pRdOut->SetPixelShader(pPS);
                pRdOut->SetTexture(pTex);
                pRdOut->SetTopology(pTopology);
                pRdOut->SetRenderLayer(Engine::RENDER_LAYER::UI);
            }
        };

        makeQuad("bg",   HPBar_detail::kFullW, pTexBG,   m_pTransformBG,   m_pRendererBG);
        makeQuad("fill", HPBar_detail::kFullW, pTexFill, m_pTransformFill, m_pRendererFill);

        return true;
    }

    void HPBar::Update(float fDeltaTime)
    {
        // Ticks child Components — Transforms (PostUpdate rebuilds the
        // matrices) and UIRenderers (PreDraw self-registers each frame).
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer || !m_pTransformFill) return;

        const float fMax = static_cast<float>(pPlayer->GetMaxHP());
        if (fMax <= 0.f) return;
        const float fRatio = std::max(0.f, std::min(1.f,
            static_cast<float>(pPlayer->GetHP()) / fMax));

        // Rewrite the fill quad's scale to track the current HP. BG
        // stays at kFullW so the empty portion shows the grey track.
        m_pTransformFill->SetScale(
            HPBar_detail::kFullW * fRatio,
            HPBar_detail::kHeight,
            1.f);
    }

    std::shared_ptr<Engine::Component> HPBar::Clone()
    {
        return std::make_shared<HPBar>(*this);
    }
}

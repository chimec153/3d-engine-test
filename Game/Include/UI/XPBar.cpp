#include "XPBar.h"
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
    // Named namespace (not anonymous) so the Unity build doesn't merge
    // these short-named constants with HPBar's / EnemyCountHUD's. See
    // HPBar.cpp's HPBar_detail comment for the long version.
    namespace XPBar_detail
    {
        // Bar slot just above HPBar (which sits at y=-0.95, height 0.05).
        constexpr float kBaseX  = -0.95f;
        constexpr float kBaseY  = -0.89f;   // 0.01 gap above HPBar
        constexpr float kHeight =  0.025f;
        constexpr float kFullW  =  0.4f;

        std::shared_ptr<Engine::Texture> EnsureBGTexture()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("XPBarBG")) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>("XPBarBG");
            if (!pNew) return nullptr;
            const uint32_t grey = 0xFF202020;
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &grey;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        std::shared_ptr<Engine::Texture> EnsureFillTexture()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("XPBarFill")) return p;
            auto pNew = Engine::StaticCreateBindable<Engine::Texture>("XPBarFill");
            if (!pNew) return nullptr;
            // ABGR memory layout: A=FF, B=20, G=D0, R=E0 → warm yellow.
            const uint32_t yellow = 0xFF20D0E0;
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &yellow;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }
    }

    XPBar::XPBar()
        : Engine::UIControl()
    {
        Engine::Component::SetComponentType(Engine::COMPONENT_TYPE::NONE);
    }

    bool XPBar::Init()
    {
        if (!Engine::UIControl::Init()) return false;

        auto pVS       = Engine::StaticFindBindable<Engine::VertexShader>("UIVS");
        auto pPS       = Engine::StaticFindBindable<Engine::PixelShader> ("UIPS");
        auto pTopology = Engine::StaticFindBindable<Engine::Topology>    ("TriangleStrip");
        auto pMesh     = Engine::StaticFindBindable<Engine::Mesh>        ("UIQuad");
        auto pTexBG    = XPBar_detail::EnsureBGTexture();
        auto pTexFill  = XPBar_detail::EnsureFillTexture();
        if (!pVS || !pPS || !pTopology || !pMesh || !pTexBG || !pTexFill) return false;

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
                pTrOut->SetCameraType(Engine::CAMERA_TYPE::UI);
                pTrOut->SetScale   (fWidth,                  XPBar_detail::kHeight, 1.f);
                pTrOut->SetPosition(XPBar_detail::kBaseX,    XPBar_detail::kBaseY,  0.f);
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

        makeQuad("bg",   XPBar_detail::kFullW, pTexBG,   m_pTransformBG,   m_pRendererBG);
        makeQuad("fill", XPBar_detail::kFullW, pTexFill, m_pTransformFill, m_pRendererFill);

        return true;
    }

    void XPBar::Update(float fDeltaTime)
    {
        Engine::UIControl::Update(fDeltaTime);

        auto pPlayer = m_pTarget.lock();
        if (!pPlayer || !m_pTransformFill) return;

        const float fToNext = static_cast<float>(pPlayer->GetXpToNext());
        if (fToNext <= 0.f) return;
        const float fRatio = std::max(0.f, std::min(1.f,
            static_cast<float>(pPlayer->GetExp()) / fToNext));

        m_pTransformFill->SetScale(
            XPBar_detail::kFullW * fRatio,
            XPBar_detail::kHeight,
            1.f);
    }

    std::shared_ptr<Engine::Component> XPBar::Clone()
    {
        return std::make_shared<XPBar>(*this);
    }
}

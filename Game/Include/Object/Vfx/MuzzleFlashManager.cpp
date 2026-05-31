#include "MuzzleFlashManager.h"
#include "Bindable/Transform.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/InputLayout.h"
#include "Bindable/Topology.h"
#include "Bindable/RasterizerState.h"
#include "Bindable/BlendState.h"
#include "Bindable/DepthStencilState.h"
#include "Bindable/VertexBuffer.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Core/Macro.h"
#include "Types.h"
#include <cmath>

namespace Client { MuzzleFlashManager* MuzzleFlashManager::m_pInst = nullptr; }

namespace Client
{
    // Named namespace (Game is a unity/jumbo build -- avoid anon-namespace
    // collisions across the merged TU).
    namespace muzzle_flash_detail
    {
        constexpr int   kPool  = 64;       // round-robin flashes (very short life)
        constexpr float kLife  = 0.06f;    // ~one frame visible, then gone
        constexpr float kSize  = 1.4f;     // base diameter in world units
        constexpr float kElongF = 0.80f;   // forward half-extent factor (long axis)
        constexpr float kElongS = 0.38f;   // side half-extent factor (short axis)
        constexpr float kHDR   = 3.0f;     // tint scaled > 1 so the core blooms
        // Hot white-yellow flash, matching the legacy particle muzzle's start
        // colour. The life envelope and kHDR multiply this on the CPU.
        constexpr float kColR  = 1.00f;
        constexpr float kColG  = 0.95f;
        constexpr float kColB  = 0.70f;
    }

    float MuzzleFlashManager::Rand()
    {
        m_uSeed = m_uSeed * 1664525u + 1013904223u;
        return static_cast<float>(m_uSeed >> 8) * (1.f / 16777216.f);
    }

    bool MuzzleFlashManager::Init()
    {
        using namespace muzzle_flash_detail;
        m_flashes.assign(kPool, Flash{});
        m_iNext = 0;

        if (!m_bInit)
        {
            m_pVS           = Engine::StaticFindBindable<Engine::VertexShader>("BeamVS");
            m_pPS           = Engine::StaticFindBindable<Engine::PixelShader>("MuzzlePS");
            m_pIL           = Engine::StaticFindBindable<Engine::InputLayout>("BeamVtx");
            m_pTopo         = Engine::StaticFindBindable<Engine::Topology>("TriangleList");
            m_pNoCull       = Engine::StaticFindBindable<Engine::RasterizerState>(CULL_NONE);
            m_pAddBlend     = Engine::StaticFindBindable<Engine::BlendState>("AccBlend");
            m_pNoDepthWrite = Engine::StaticFindBindable<Engine::DepthStencilState>("NoDepthWrite");

            m_pVP = std::make_shared<Engine::Transform>();
            m_pVP->SetCameraType(Engine::CAMERA_TYPE::NORMAL);   // world WVP; identity world -> g_matTransform = VP

            Engine::BindableRegistry::Register([]() { MuzzleFlashManager::DestroyInst(); });
            m_bInit = true;
        }
        return m_pVS && m_pPS && m_pIL && m_pTopo;
    }

    void MuzzleFlashManager::Spawn(const Engine::Vector3& vPos, const Engine::Vector3& vDir)
    {
        using namespace muzzle_flash_detail;
        if (m_flashes.empty()) return;

        // Normalise the fire direction; fall back to +X for a zero vector so a
        // bad call still draws something sensible.
        Engine::Vector3 d = vDir;
        const float len = d.Length();
        d = (len > 1e-4f) ? d * (1.f / len) : Engine::Vector3(1.f, 0.f, 0.f);

        Flash& f = m_flashes[m_iNext];
        m_iNext = (m_iNext + 1) % static_cast<int>(m_flashes.size());
        f.vPos    = vPos;
        f.vDir    = d;
        f.fSize   = kSize * (0.85f + 0.3f * Rand());   // slight per-shot variation
        f.fSeed   = Rand();                            // jitters the forward spikes
        f.fLife   = kLife;
        f.fAge    = 0.f;
        f.bActive = true;
    }

    void MuzzleFlashManager::Update(float fDeltaTime)
    {
        for (Flash& f : m_flashes)
        {
            if (!f.bActive) continue;
            f.fAge += fDeltaTime;
            if (f.fAge >= f.fLife) f.bActive = false;
        }
    }

    void MuzzleFlashManager::BuildQuad(std::vector<MuzzleVertex>& out,
        const Engine::Vector3& vCentre, float fHalfF, float fHalfS,
        const Engine::Vector3& vAxisF, const Engine::Vector3& vAxisS,
        float tintR, float tintG, float tintB, float fSeed)
    {
        // uv.x increases along +forward, uv.y along +side.
        const Engine::Vector3 fwd = vAxisF * fHalfF;
        const Engine::Vector3 side = vAxisS * fHalfS;
        const Engine::Vector3 c0 = vCentre - fwd - side;   // uv (0,0)
        const Engine::Vector3 c1 = vCentre + fwd - side;   // uv (1,0)
        const Engine::Vector3 c2 = vCentre + fwd + side;   // uv (1,1)
        const Engine::Vector3 c3 = vCentre - fwd + side;   // uv (0,1)
        auto push = [&](const Engine::Vector3& p, float uu, float vv)
        {
            out.push_back({ p.x, p.y, p.z, uu, vv, tintR, tintG, tintB, fSeed });
        };
        push(c0, 0.f, 0.f); push(c1, 1.f, 0.f); push(c2, 1.f, 1.f);
        push(c0, 0.f, 0.f); push(c2, 1.f, 1.f); push(c3, 0.f, 1.f);
    }

    void MuzzleFlashManager::Render()
    {
        using namespace muzzle_flash_detail;
        if (!m_bInit || !m_pVS || !m_pPS || !m_pIL || !m_pTopo || !m_pVP ||
            !m_pNoCull || !m_pAddBlend || !m_pNoDepthWrite)
            return;

        // Screen-facing billboard basis from the camera.
        Engine::Vector3 vRight(1.f, 0.f, 0.f), vUp(0.f, 1.f, 0.f);
        if (auto pCam = Engine::Graphics::GetInst()->GetCamera())
            if (auto pTr = pCam->GetTransform())
            {
                vRight = pTr->GetAxis(Engine::AXIS_TYPE::X);
                vUp    = pTr->GetAxis(Engine::AXIS_TYPE::Y);
            }

        m_verts.clear();
        for (const Flash& f : m_flashes)
        {
            if (!f.bActive) continue;
            const float t    = f.fAge / f.fLife;           // 0..1
            const float env  = (1.f - t) * (1.f - t);      // fast-decay brightness
            const float grow = 0.9f + 0.25f * t;           // slight expand over life

            // Project the world fire direction onto the camera-facing billboard
            // plane to get the on-screen "forward", then a perpendicular side
            // axis. The quad is longer along forward (kElongF) than across
            // (kElongS) so the flash reads as a directional blast.
            float fF = f.vDir.x * vRight.x + f.vDir.y * vRight.y + f.vDir.z * vRight.z;
            float fU = f.vDir.x * vUp.x    + f.vDir.y * vUp.y    + f.vDir.z * vUp.z;
            float fl = sqrtf(fF * fF + fU * fU);
            if (fl < 1e-4f) { fF = 1.f; fU = 0.f; fl = 1.f; }   // aimed at/away from camera
            fF /= fl; fU /= fl;
            const Engine::Vector3 axisF = vRight * fF + vUp * fU;       // forward (in plane)
            const Engine::Vector3 axisS = vRight * (-fU) + vUp * fF;    // perpendicular side

            BuildQuad(m_verts, f.vPos, f.fSize * grow * kElongF, f.fSize * grow * kElongS,
                      axisF, axisS,
                      kColR * kHDR * env, kColG * kHDR * env, kColB * kHDR * env,
                      f.fSeed);
        }
        if (m_verts.empty()) return;

        auto* pDC = Engine::Graphics::GetInst()->GetDeviceContext();

        m_pAddBlend->Bind();        // additive ONE/ONE (restores AlphaBlend on PostBind)
        m_pNoDepthWrite->Bind();    // depth test on, write off
        m_pNoCull->Bind();          // billboard winding flips with view

        m_pVP->SetScale(1.f, 1.f, 1.f);
        m_pVP->SetPosition(0.f, 0.f, 0.f);
        m_pVP->PostUpdate(0.f);
        m_pVP->Bind();

        m_pVS->Bind();
        m_pPS->Bind();
        m_pIL->Bind();
        m_pTopo->Bind();

        auto pVB = std::make_shared<Engine::VertexBuffer>(m_verts);
        pVB->Bind();
        pDC->Draw(static_cast<UINT>(m_verts.size()), 0);

        m_pNoCull->PostBind();
        m_pNoDepthWrite->PostBind();
        m_pAddBlend->PostBind();

        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }

    void MuzzleFlashManager::Clear()
    {
        for (Flash& f : m_flashes) f.bActive = false;
        m_iNext = 0;
    }
}

#include "VfxManager.h"
#include "Bindable/Particle.h"
#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Bindable/Transform.h"
#include "GameObject/GameObject.h"
#include "Types.h"
#include <string>

namespace Client
{
    VfxManager* VfxManager::m_pInst = nullptr;

    namespace
    {
        std::shared_ptr<Engine::Texture> EnsureParticleTex()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("particletexture"))
                return p;
            return Engine::StaticCreateBindable<Engine::Texture>(
                "particletexture", "/Game/Texture/Particle/particle_00.png", TEXTURE_PATH);
        }

        // Burn flames use a 4x4 (16-frame) grayscale flipbook. Grayscale so the
        // shader's per-particle colour multiply (PS_PARTICLE) tints it warm.
        std::shared_ptr<Engine::Texture> EnsureFireTex()
        {
            if (auto p = Engine::StaticFindBindable<Engine::Texture>("firetexture"))
                return p;
            return Engine::StaticCreateBindable<Engine::Texture>(
                "firetexture", "/Game/Texture/Particle/fire.png", TEXTURE_PATH);
        }

        // Burst a pool entry at vPos: reposition the emitter, then queue iCount
        // particles. Returns once a live emitter has been bursted.
        void BurstNext(std::vector<std::weak_ptr<Engine::Particle>>& pool,
                       int& iNext, const Engine::Vector3& vPos, int iCount)
        {
            const int n = static_cast<int>(pool.size());
            if (n == 0) return;
            for (int k = 0; k < n; ++k)
            {
                auto p = pool[iNext % n].lock();
                iNext = (iNext + 1) % n;
                if (!p) continue;
                if (auto tr = p->GetTransform()) tr->SetPosition(vPos);
                p->AddEmitCount(iCount);
                return;
            }
        }
    }

    void VfxManager::Setup(Engine::GameObject* pHost)
    {
        // Destroy the singleton at app shutdown (Client/Editor mains call
        // BindableRegistry::DestroyAll). Registered once; the callback deletes
        // whatever instance is live then. Mirrors DamageTextManager's hook.
        static bool s_bRegistered = false;
        if (!s_bRegistered)
        {
            Engine::BindableRegistry::Register([]() { VfxManager::DestroyInst(); });
            s_bRegistered = true;
        }

        m_hitPool.clear();
        m_deathPool.clear();
        m_burnPool.clear();
        m_iHitNext = m_iDeathNext = m_iBurnNext = 0;
        if (!pHost) return;

        auto pTex = EnsureParticleTex();

        // Hit spark - punchy little burst so a non-killing hit reads clearly
        // (still smaller / shorter than the death flare).
        for (int i = 0; i < kHitEmitters; ++i)
        {
            auto p = pHost->AddComponent<Engine::Particle>("vfx_hit_" + std::to_string(i), 128);
            if (!p) continue;
            p->SetTexture(pTex);
            p->SetEmitTime(0.0001f);                 // tiny interval => instant burst
            p->SetMaxLifeTime(0.5f);
            p->SetStartColor({ 1.f, 0.95f, 0.6f, 1.f });   // bright spark
            p->SetEndColor  ({ 1.f, 0.45f, 0.1f, 0.f });
            p->SetStartSize ({ 0.18f, 0.18f });
            p->SetEndSize   ({ 0.04f, 0.04f });
            p->SetMinCreatePosition({ -0.12f, 0.f,  -0.12f });
            p->SetMaxCreatePosition({  0.12f, 0.3f,  0.12f });
            p->SetVelocity   ({ -3.f, 1.5f, -3.f });
            p->SetMaxVelocity({  3.f, 5.f,  3.f });
            p->SetAccelaration({ 0.f, -7.f, 0.f });
            p->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
            p->AddEmitCount(1);   // arm: flip from infinite (-1) to finite-idle (0)
            m_hitPool.push_back(p);
        }

        // Death flare — bigger, longer, redder.
        for (int i = 0; i < kDeathEmitters; ++i)
        {
            auto p = pHost->AddComponent<Engine::Particle>("vfx_death_" + std::to_string(i), 384);
            if (!p) continue;
            p->SetTexture(pTex);
            p->SetEmitTime(0.0001f);
            p->SetMaxLifeTime(1.1f);
            // Hot core fading to ember red, bigger and longer than a hit so a
            // kill reads clearly.
            p->SetStartColor({ 1.f, 0.85f, 0.45f, 1.f });
            p->SetEndColor  ({ 0.7f, 0.1f,  0.f,  0.f });
            p->SetStartSize ({ 0.30f, 0.30f });
            p->SetEndSize   ({ 0.06f, 0.06f });
            p->SetMinCreatePosition({ -0.2f, 0.f,  -0.2f });
            p->SetMaxCreatePosition({  0.2f, 0.4f,  0.2f });
            p->SetVelocity   ({ -4.5f, 2.f, -4.5f });
            p->SetMaxVelocity({  4.5f, 7.5f, 4.5f });
            p->SetAccelaration({ 0.f, -8.f, 0.f });
            p->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
            p->AddEmitCount(1);
            m_deathPool.push_back(p);
        }

        // Burn flame — a flipbook flame sprite that clings to the burning body
        // and licks upward. Emitted on a fast sub-tick by Enemy while burning,
        // so a small per-burst count reads as a continuous column. The 4x4
        // sheet plays once across each particle's lifetime (frame = age/maxage
        // * MaxFrame in CS_PARTICLE), so MaxLifeTime sets the flicker speed.
        auto pFireTex = EnsureFireTex();
        for (int i = 0; i < kBurnEmitters; ++i)
        {
            auto p = pHost->AddComponent<Engine::Particle>("vfx_burn_" + std::to_string(i), 128);
            if (!p) continue;
            p->SetTexture(pFireTex);
            p->SetFrameWidth(4);                     // 4 columns
            p->SetFrameHeight(4);                    // 4 rows (square sheet)
            p->SetMaxFrame(16);                      // 16 frames played over life
            p->SetEmitTime(0.0001f);                 // tiny interval => instant burst
            p->SetMaxLifeTime(0.55f);
            // Grayscale sheet is tinted here: bright orange core fading to ember.
            p->SetStartColor({ 1.f, 0.75f, 0.25f, 1.f });
            p->SetEndColor  ({ 0.5f, 0.08f, 0.f,   0.f });
            p->SetStartSize ({ 0.40f, 0.55f });      // flames taller than wide
            p->SetEndSize   ({ 0.18f, 0.26f });
            p->SetMinCreatePosition({ -0.18f, -0.1f, -0.18f });
            p->SetMaxCreatePosition({  0.18f,  0.3f,  0.18f });
            p->SetVelocity   ({ -0.4f, 0.6f, -0.4f });
            p->SetMaxVelocity({  0.4f, 1.8f,  0.4f });
            p->SetAccelaration({ 0.f, 0.8f, 0.f });  // buoyant rise
            p->SetRenderLayer(Engine::RENDER_LAYER::ALPHA);
            p->AddEmitCount(1);                      // arm
            m_burnPool.push_back(p);
        }
    }

    void VfxManager::SpawnHit(const Engine::Vector3& vPos)
    {
        BurstNext(m_hitPool, m_iHitNext, vPos, kHitBurst);
    }

    void VfxManager::SpawnDeath(const Engine::Vector3& vPos)
    {
        BurstNext(m_deathPool, m_iDeathNext, vPos, kDeathBurst);
    }

    void VfxManager::SpawnBurn(const Engine::Vector3& vPos)
    {
        BurstNext(m_burnPool, m_iBurnNext, vPos, kBurnBurst);
    }
}

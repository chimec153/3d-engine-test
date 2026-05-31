#pragma once
#include "Vector3.h"
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>

namespace Engine
{
    class Particle;
    class GameObject;
}

namespace Client
{
    // Shared particle-burst pool. A per-enemy Particle component is expensive
    // here — each one is a per-frame CS dispatch + system-buffer upload, which
    // dominated the profile when enemies each carried one (see Attackable's
    // blood-particle note). Instead a fixed handful of emitters live on a
    // single host GameObject; SpawnHit / SpawnDeath reposition the next emitter
    // and burst it, so the cost is constant regardless of enemy count.
    //
    // Singleton (new / DestroyInst, like the engine singletons). Enemy reaches
    // it without threading a pointer through. The emitters are owned by the
    // host GameObject (the pool holds weak_ptr), so a scene change
    // auto-invalidates them; the manager itself persists until DestroyInst
    // (wired to BindableRegistry shutdown in Setup).
    class VfxManager
    {
        static VfxManager* m_pInst;

    public:
        static VfxManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = dbg_new VfxManager;
            return m_pInst;
        }

        static void DestroyInst()
        {
            if (m_pInst)
            {
                delete m_pInst;
                m_pInst = nullptr;
            }
        }

        // Create + configure the emitter pool as components on pHost. Called
        // once per scene by GameScene; re-registers fresh each time.
        void Setup(Engine::GameObject* pHost);

        // Burst a small spark at a world position (enemy hit) or a larger
        // flare (enemy death). No-op if Setup hasn't run / the host is gone.
        void SpawnHit(const Engine::Vector3& vPos);
        void SpawnDeath(const Engine::Vector3& vPos);
        // Burst a small flipbook flame at a world position. Called repeatedly on
        // a sub-tick by a burning enemy so the flames read as a steady column
        // following the body (Enemy::Update). No-op if Setup hasn't run.
        void SpawnBurn(const Engine::Vector3& vPos);

    private:
        VfxManager() = default;
        ~VfxManager() = default;
        VfxManager(const VfxManager&) = delete;
        VfxManager& operator=(const VfxManager&) = delete;

        // Round-robin pools. weak_ptr: the host GameObject owns lifetime.
        std::vector<std::weak_ptr<Engine::Particle>> m_hitPool;
        std::vector<std::weak_ptr<Engine::Particle>> m_deathPool;
        std::vector<std::weak_ptr<Engine::Particle>> m_burnPool;
        int m_iHitNext   = 0;
        int m_iDeathNext = 0;
        int m_iBurnNext  = 0;

        static constexpr int kHitEmitters   = 6;
        static constexpr int kDeathEmitters = 2;
        static constexpr int kBurnEmitters  = 6;
        static constexpr int kHitBurst      = 30;   // particles per hit
        static constexpr int kDeathBurst    = 140;  // particles per death
        static constexpr int kBurnBurst     = 3;    // flames per burn sub-tick
    };
}

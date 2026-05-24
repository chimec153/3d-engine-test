#pragma once
#include <memory>
#include <vector>

namespace Engine
{
    class Scene;
    class VoxelWorld;
    class GameObject;
    class Transform;
    class Layer;
}

namespace Client
{
    class FlowField;

    // Per-frame enemy spawning. Owns one accumulator per SpawnConfig
    // rule so parallel waves (e.g. row 0 = Box every 0.5s for 30s,
    // row 1 = Capsule every 1s for the same window) each tick on
    // their own cadence. The scene's Update body shrinks to a single
    // Tick(dt) call.
    //
    // The spawner also owns the shared FlowField: it rebuilds the field
    // whenever the player crosses into a new cell and injects the field
    // pointer into every freshly-spawned Enemy so the army shares one
    // Dijkstra solution instead of running per-enemy A*.
    class EnemySpawner
    {
    public:
        EnemySpawner(Engine::Scene* pScene, Engine::VoxelWorld* pWorld);
        ~EnemySpawner();

        void Tick(float fDeltaTime);

    private:
        Engine::Scene*       m_pScene = nullptr;
        Engine::VoxelWorld*  m_pWorld = nullptr;

        std::unique_ptr<FlowField> m_pFlowField;

        // Per-SpawnConfig-rule runtime state. The vector is grown
        // lazily in Tick to match the current rule count (so editing
        // spawn.csv mid-run picks up new rules automatically).
        struct RuleState
        {
            float fSpawnAcc = 0.f;   // accumulator vs the rule's interval
            int   iSpawnIdx = 0;     // round-robin index (used when enemy_id ≤ 0)
        };
        std::vector<RuleState> m_vecRuleStates;

        // Scene-elapsed seconds since the spawner came online. Compared
        // against each rule's first/last spawn-time window.
        float m_fElapsed  = 0.f;

        // One-call spawn: builds a single enemy off the supplied rule's
        // angle/radius and applies its EnemyDef. Factored out so the
        // per-rule loop in Tick stays readable.
        void SpawnOne(struct SpawnRule const& rule, RuleState& state,
                      const std::shared_ptr<Engine::GameObject>& pPlayer,
                      const std::shared_ptr<Engine::Transform>& pPlayerTr,
                      const std::shared_ptr<Engine::Layer>& pLayer);
    };
}

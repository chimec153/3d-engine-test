#pragma once
#include <memory>
#include <string>
#include <utility>
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
    struct RoundDef;
    struct RoundSpawn;

    // Drives the data-driven round system.
    //
    // - StartRound(N) looks up RoundDef N in RoundDatabase, captures the
    //   round multipliers / duration, and resets per-spawn accumulators.
    // - Tick(dt) advances the round timer and, for each spawn entry in
    //   the current round, accumulates against its interval inside its
    //   [startTime, endTime) window and queues a burst on each elapse.
    // - The queue plays a ground-ring telegraph for kTelegraphTime before
    //   the actual enemy materialises (same flow as the previous
    //   round-robin path).
    //
    // The spawner also owns the shared FlowField: it rebuilds the field
    // whenever the player crosses into a new cell and injects the field
    // pointer into every freshly-spawned Enemy so the whole army shares
    // one Dijkstra solution instead of running per-enemy A*.
    class EnemySpawner
    {
    public:
        EnemySpawner(Engine::Scene* pScene, Engine::VoxelWorld* pWorld);
        ~EnemySpawner();

        void Tick(float fDeltaTime);

        // Round/wave control — JSON-driven. RoundDatabase.Get(iRound) is
        // captured at Start; lookup failure leaves the round inactive.
        // GameScene drives the Playing → Intermission → next-round loop.
        void StartRound(int iRound);
        void EndRound();
        bool IsRoundActive()   const { return m_bRoundActive; }
        bool IsRoundComplete() const;
        int  GetRound()        const { return m_iRound; }
        int  GetAliveEnemyCount() const;
        // Seconds left in the current round (0 when not active). HUD readout.
        float GetRoundTimeRemaining() const;
        // Deactivate every live enemy — called when a round is survived so the
        // next round starts on a clean field.
        void ClearEnemies();

    private:
        Engine::Scene*       m_pScene = nullptr;
        Engine::VoxelWorld*  m_pWorld = nullptr;

        std::unique_ptr<FlowField> m_pFlowField;

        // Goal the shared field is currently committed to. Kept across ticks so
        // we don't re-search candidates (and rebuild the field per candidate)
        // every frame — a stationary committed tower's Rebuild is then a cached
        // no-op. Weak so a destroyed tower drops out and forces a re-search.
        std::weak_ptr<Engine::GameObject> m_wpCommittedGoal;

        // Per-spawn-entry runtime accumulator. One element per
        // m_pCurrentRound->vecSpawns entry. Reset on StartRound.
        struct SpawnState
        {
            float fAcc = 0.f;     // seconds since the last burst for this entry
        };
        std::vector<SpawnState> m_vecSpawnStates;

        // A spawn that has been pre-rolled (cell + EnemyDef key decided)
        // and is currently showing a ground telegraph. When fAge reaches
        // kTelegraphTime the enemy materialises. The string id + round
        // multipliers are captured at queue-time so a mid-flight round
        // change (theoretical) can't poison the materialisation.
        struct PendingSpawn
        {
            int          cx       = 0;
            int          cz       = 0;
            std::string  strEnemyId;
            float        fHpMult  = 1.f;
            float        fDmgMult = 1.f;
            float        fAge     = 0.f;
        };
        std::vector<PendingSpawn> m_vecPending;
        // Telegraph length (seconds) — the red ring fills in this much time
        // before the enemy actually appears.
        static constexpr float kTelegraphTime   = 0.8f;
        // Telegraph ring radius (world units) — sized to a spawn cell.
        static constexpr float kTelegraphRadius = 0.55f;
        // Ring distance from the player for edge_random / edge_all /
        // point_burst patterns. Just outside the screen-edge feel.
        static constexpr float kEdgeRadius      = 10.f;
        // Tighter radius for the `ring` pattern so it surrounds the player
        // visibly rather than reading as another edge spawn.
        static constexpr float kRingRadius      = 5.f;
        // Concurrent-alive ceiling. Rounds 16+ pump out swarmlings fast;
        // a cap stops the field from snowballing into an unwinnable / slow
        // frame. Bumped from the old round-robin path (30) because the
        // new spawn table is denser by design.
        static constexpr int   kMaxAliveEnemies = 80;

        // Current round captured at Start.
        const RoundDef* m_pCurrentRound = nullptr;
        int             m_iRound        = 0;
        float           m_fRoundElapsed = 0.f;
        bool            m_bRoundActive  = false;
        // One boss per boss round — set true after MaterialiseBoss so the
        // tick loop doesn't keep re-spawning it. Reset in StartRound.
        bool            m_bBossSpawned  = false;

        // Boss is materialised directly (no telegraph) when the round
        // elapsed crosses fSpawnTime. Picks a point on the same edge ring
        // as point_burst spawns; falls back through a few angles if the
        // first cell is solid.
        void MaterialiseBoss(const std::shared_ptr<Engine::GameObject>& pPlayer,
                             const std::shared_ptr<Engine::Layer>& pLayer,
                             const std::shared_ptr<Engine::Transform>& pPlayerTr);

        // Pick a (cx, cz) for one enemy in a burst at index `iSlot` of
        // `iBurst`, apply the pattern's placement maths, validate
        // wall-cell rejection, and push a PendingSpawn the telegraph
        // loop will draw + materialise.
        void QueueOne(const RoundSpawn& spawn, int iSlot,
                      const std::shared_ptr<Engine::Transform>& pPlayerTr);

        // Build the actual Enemy at the pending's cell, apply its
        // captured def + multipliers. Skips if the cell has since become
        // solid (player built a wall on the telegraphed cell during the
        // warning window).
        void MaterialisePending(const PendingSpawn& pending,
                                const std::shared_ptr<Engine::GameObject>& pPlayer,
                                const std::shared_ptr<Engine::Layer>& pLayer);

        // Reachability probe for goal selection: writes the first live enemy's
        // cell into (cx, cz). Returns false when no enemy is alive.
        bool FindProbeEnemy(Engine::Layer* pLayer, int& cx, int& cz) const;

        // Pick the strongest-aggro target the enemies can actually path to,
        // using a live-enemy cell as the reachability probe. Builds the shared
        // field to the winner and returns it; when the real target is walled
        // off this yields the nearest reachable wall tower so enemies break it
        // down. Falls back to the player / strongest target when nothing is
        // reachable.
        std::shared_ptr<Engine::GameObject> SelectReachableGoal(
            Engine::Layer* pLayer,
            const std::vector<std::pair<int, int>>& vecBlocked,
            int probeCx, int probeCz, bool bProbe,
            const std::shared_ptr<Engine::GameObject>& pPlayer);
    };
}

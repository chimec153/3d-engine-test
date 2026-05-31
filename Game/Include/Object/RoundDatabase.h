#pragma once

#include <string>
#include <vector>

namespace Client
{
    // How a spawn entry distributes its `burst` enemies around the player.
    // Matches the four pattern strings in rounds.json:
    //   edge_random — one random angle on the spawn ring; the whole burst
    //                 lands at the same point (single-point pressure).
    //   edge_all    — `burst` evenly-spaced angles → spread across all
    //                 sides at once.
    //   point_burst — same as edge_random visually; kept as a distinct
    //                 enum so the spawner can later add a slightly tighter
    //                 cluster offset without changing data.
    //   ring        — evenly-spaced angles on a SHORTER radius so the
    //                 burst forms a visible ring around the player.
    enum class SpawnPattern
    {
        EdgeRandom,
        EdgeAll,
        PointBurst,
        Ring,
    };

    // One spawn entry inside a round. JSON shape:
    //   { "enemyId": "crawler", "startTime": 0, "endTime": 30,
    //     "interval": 2.0, "burst": 1, "pattern": "edge_random" }
    struct RoundSpawn
    {
        std::string  strEnemyId;
        float        fStartTime = 0.f;
        float        fEndTime   = 0.f;
        float        fInterval  = 2.f;
        int          iBurst     = 1;
        SpawnPattern ePattern   = SpawnPattern::EdgeRandom;
    };

    // Phase 1: boss data is parsed and stored but the spawner doesn't
    // materialise it — round 10 / 20 still run their listed swarmling /
    // phantom spawns, but no boss enemy appears. Phase 2 will wire this.
    struct RoundBoss
    {
        std::string  strBossId;       // "devourer" / "hive_queen"
        float        fSpawnTime = 0.f;
        SpawnPattern ePattern   = SpawnPattern::PointBurst;
    };

    // One row in rounds.json. The spawner reads `fDuration` for the
    // survival timer, walks `vecSpawns` every tick within each entry's
    // [startTime, endTime) window, and multiplies enemy stats by the
    // hp/damage multipliers at materialisation time.
    struct RoundDef
    {
        int  iRound = 0;
        float fDuration         = 30.f;
        bool  bIsBoss           = false;
        float fHpMultiplier     = 1.f;
        float fDamageMultiplier = 1.f;
        std::vector<RoundSpawn> vecSpawns;
        RoundBoss tBoss;   // only meaningful if bIsBoss
    };

    // Static catalogue of every round. Loaded once from rounds.json at
    // scene init. Singleton — matches EnemyDatabase / SpawnConfig.
    class RoundDatabase
    {
    public:
        static RoundDatabase& GetInst()
        {
            static RoundDatabase inst;
            return inst;
        }

        size_t LoadFromJSON(const std::string& strPath);

        // Lookup by round number (1-indexed). Returns nullptr if not
        // loaded / past the last round.
        const RoundDef* Get(int iRound) const;

        size_t Count() const { return m_vecRounds.size(); }
        // Total round count from config (defaults to vector size if not
        // specified). HUD / GameOver can show "Round X of N".
        int    GetTotalRounds() const { return m_iTotalRounds; }

    private:
        RoundDatabase() = default;
        RoundDatabase(const RoundDatabase&)            = delete;
        RoundDatabase& operator=(const RoundDatabase&) = delete;

        std::vector<RoundDef> m_vecRounds;
        int                   m_iTotalRounds = 0;
    };
}

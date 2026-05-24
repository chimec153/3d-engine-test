#pragma once

#include <string>
#include <vector>

namespace Client
{
    // One spawn rule loaded from a single row of spawn.csv. Multiple
    // rules let the file describe waves (e.g. row 1 = Box every 0.5s
    // for the first 30s, row 2 = Capsule every 1.0s for the same
    // window, row 3 = a tougher Blue Box from 30s onward, …). All
    // rules run in parallel — EnemySpawner ticks each one against its
    // own time window and interval accumulator.
    struct SpawnRule
    {
        float fInterval     = 1.0f;   // seconds between spawns
        float fRadius       = 8.0f;   // cells from the player
        float fFirstTime    = 0.0f;   // scene-elapsed grace period
        float fLastTime     = 0.0f;   // 0 or negative = no end
        int   iEnemyId      = 0;      // ≤ 0 = round-robin
    };

    // Global enemy-spawn config. Loaded once from spawn.csv at scene
    // init. Singleton so EnemySpawner — and any future debug UI — can
    // read the live rules without threading a pointer through.
    //
    // Rules stay at their built-in defaults (one rule, interval 1s,
    // radius 8, round-robin) if the file is missing or malformed.
    class SpawnConfig
    {
    public:
        static SpawnConfig& GetInst()
        {
            static SpawnConfig inst;
            return inst;
        }

        // Returns the number of rules parsed. On failure the previously
        // loaded rules (or a single default rule) stay in place.
        size_t LoadFromCSV(const std::string& strPath);

        const std::vector<SpawnRule>& GetRules() const { return m_vecRules; }

    private:
        SpawnConfig() : m_vecRules(1) {}   // one default-constructed rule
        SpawnConfig(const SpawnConfig&)            = delete;
        SpawnConfig& operator=(const SpawnConfig&) = delete;

        std::vector<SpawnRule> m_vecRules;
    };
}

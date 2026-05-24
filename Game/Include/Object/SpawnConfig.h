#pragma once

#include <string>

namespace Client
{
    // Global enemy-spawn parameters. Loaded once from spawn.csv (single
    // data row) at scene-init time. Singleton so GameScene's spawn loop
    // — and any future debug toggle UI — can read the live values
    // without threading a pointer through.
    //
    // Fields stay at their built-in defaults if the file is missing.
    class SpawnConfig
    {
    public:
        static SpawnConfig& GetInst()
        {
            static SpawnConfig inst;
            return inst;
        }

        // Returns true when a row was parsed. On failure the previously
        // loaded (or default) values stay in place.
        bool LoadFromCSV(const std::string& strPath);

        float GetEnemyInterval() const { return m_fEnemyInterval; }
        float GetEnemyRadius()   const { return m_fEnemyRadius;   }

    private:
        SpawnConfig() = default;
        SpawnConfig(const SpawnConfig&)            = delete;
        SpawnConfig& operator=(const SpawnConfig&) = delete;

        float m_fEnemyInterval = 1.0f;   // seconds between spawns
        float m_fEnemyRadius   = 8.0f;   // cells from the player
    };
}

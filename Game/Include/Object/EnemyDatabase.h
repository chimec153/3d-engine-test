#pragma once

#include "EnemyData.h"
#include <unordered_map>
#include <vector>

namespace Client
{
    // Static catalogue of every enemy variant the game can spawn. Loaded
    // once from enemies.json at scene-init time. Singleton so the round
    // spawner and any debug HUD can pull rows without threading a pointer.
    //
    // Keyed by both:
    //   - the string id ("crawler") used by rounds.json spawns[], and
    //   - a dense integer id (1..N) assigned in load order for legacy lookups
    //     and the round-robin fallback path. New code should prefer the
    //     string lookup.
    class EnemyDatabase
    {
    public:
        static EnemyDatabase& GetInst()
        {
            static EnemyDatabase inst;
            return inst;
        }

        // Returns the number of rows successfully parsed. On failure
        // (file missing / malformed) the database stays empty and the
        // spawn loop has nothing to materialise (silent no-op).
        size_t LoadFromJSON(const std::string& strPath);

        const EnemyDef*               Get(int iId) const;
        const EnemyDef*               Get(const std::string& strIdKey) const;
        const std::vector<EnemyDef>&  All() const   { return m_vecEnemies; }
        size_t                        Count() const { return m_vecEnemies.size(); }

    private:
        EnemyDatabase() = default;
        EnemyDatabase(const EnemyDatabase&)            = delete;
        EnemyDatabase& operator=(const EnemyDatabase&) = delete;

        std::vector<EnemyDef>                    m_vecEnemies;
        std::unordered_map<int, size_t>          m_mapIdToIndex;
        std::unordered_map<std::string, size_t>  m_mapKeyToIndex;
    };
}

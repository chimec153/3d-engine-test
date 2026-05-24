#pragma once

#include "EnemyData.h"
#include <unordered_map>
#include <vector>

namespace Client
{
    // Static catalogue of every enemy variant the game can spawn. Loaded
    // once from a CSV at scene-init time. Mirrors the WeaponDatabase
    // pattern — singleton so the GameScene spawn loop and any future
    // editor / debug HUD can pull rows without threading a pointer.
    class EnemyDatabase
    {
    public:
        static EnemyDatabase& GetInst()
        {
            static EnemyDatabase inst;
            return inst;
        }

        // Returns the number of rows successfully parsed. On failure
        // (file missing / no rows) the database stays empty and the
        // spawn loop falls back to its built-in defaults.
        size_t LoadFromCSV(const std::string& strPath);

        const EnemyDef*               Get(int iId) const;
        const std::vector<EnemyDef>&  All() const   { return m_vecEnemies; }
        size_t                        Count() const { return m_vecEnemies.size(); }

    private:
        EnemyDatabase() = default;
        EnemyDatabase(const EnemyDatabase&)            = delete;
        EnemyDatabase& operator=(const EnemyDatabase&) = delete;

        std::vector<EnemyDef>            m_vecEnemies;
        std::unordered_map<int, size_t>  m_mapIdToIndex;
    };
}

#pragma once

#include "WeaponData.h"
#include <unordered_map>
#include <vector>

namespace Client
{
    // Static catalogue of every weapon the game can offer. Loaded once
    // from a CSV at scene-init time. Lookups go by integer id (matches
    // the `id` column in weapons.csv).
    //
    // Singleton because both Player (firing) and LevelUpChoices (card UI)
    // need to read the catalogue without threading a pointer through.
    class WeaponDatabase
    {
    public:
        static WeaponDatabase& GetInst()
        {
            static WeaponDatabase inst;
            return inst;
        }

        // Returns the number of rows successfully parsed. On failure
        // (file missing / no rows) the database stays empty and the
        // weapon system effectively no-ops.
        size_t LoadFromCSV(const std::string& strPath);

        const WeaponDef* Get(int iId) const;
        const std::vector<WeaponDef>& All() const { return m_vecWeapons; }
        size_t Count() const { return m_vecWeapons.size(); }

    private:
        WeaponDatabase() = default;
        WeaponDatabase(const WeaponDatabase&) = delete;
        WeaponDatabase& operator=(const WeaponDatabase&) = delete;

        std::vector<WeaponDef>            m_vecWeapons;
        std::unordered_map<int, size_t>   m_mapIdToIndex;
    };
}

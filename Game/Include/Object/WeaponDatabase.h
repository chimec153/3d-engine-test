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

        // Inserts a runtime-crafted weapon (WeaponComboScene). The id is
        // assigned here (max existing + 1) and returned. The def is also
        // kept in a session-persistent registry so it survives a later
        // LoadFromCSV (GameScene reloads the catalogue on entry) — see
        // the re-apply pass at the end of LoadFromCSV.
        int Add(const WeaponDef& def);

        // Crafted-weapon registry + equip loadout (WeaponComboScene).
        // Only *equipped* crafted weapons feed the in-stage level-up pool,
        // capped at kMaxEquipped. The registry is indexed in craft order;
        // those indices are stable for the session and used by the combo
        // scene's loadout UI. iLiveId tracks each entry's current id in
        // m_vecWeapons (re-assigned on every LoadFromCSV).
        static constexpr int kMaxEquipped = 10;
        int  CraftedCount() const { return static_cast<int>(m_vecCrafted.size()); }
        const WeaponDef& CraftedDef(int iIndex) const;
        bool IsEquipped(int iIndex) const;
        int  EquippedCount() const;
        // Flips the equip flag for a registry entry. Refuses to equip past
        // kMaxEquipped (leaving it unequipped). Returns the new state.
        bool ToggleEquip(int iIndex);
        // Live ids of the equipped crafted weapons — the in-stage pool.
        std::vector<int> EquippedLiveIds() const;

    private:
        WeaponDatabase() = default;
        WeaponDatabase(const WeaponDatabase&) = delete;
        WeaponDatabase& operator=(const WeaponDatabase&) = delete;

        // Smallest unused positive id given the current m_vecWeapons.
        int NextId() const;

        // One crafted weapon in the session registry.
        struct CraftedEntry
        {
            WeaponDef def;            // the crafted definition (id is transient)
            bool      bEquipped = false;
            int       iLiveId   = -1; // current id in m_vecWeapons
        };

        std::vector<WeaponDef>            m_vecWeapons;
        std::unordered_map<int, size_t>   m_mapIdToIndex;
        // Crafted weapons, kept across LoadFromCSV clears so they persist
        // for the rest of the process run (not written to disk).
        std::vector<CraftedEntry>         m_vecCrafted;
    };
}

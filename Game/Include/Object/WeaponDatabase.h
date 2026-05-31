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

        // Overwrite the loaded weapon with the given id in place (the id is
        // kept). Used by the weapon editor (WeaponComboScene) to apply edits to
        // a weapons_v2.csv weapon. Returns false if no such id is loaded.
        bool UpdateWeapon(int iId, const WeaponDef& def);

        // Write the whole loaded catalogue back out in the weapons_v2.csv v2
        // column format (enum tokens, px-scaled speed/accel, hex colour). Used
        // by the editor to persist edits. Returns rows written.
        size_t SaveToCSV(const std::string& strPath) const;

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
        // Permanently removes a crafted weapon from the registry (combo-scene
        // destroy). Shifts later indices down; the live m_vecWeapons copy is
        // dropped on the next LoadFromCSV re-apply.
        void RemoveCrafted(int iIndex);
        // Live ids of the equipped crafted weapons — the in-stage pool.
        std::vector<int> EquippedLiveIds() const;
        // Live ids of EVERY crafted weapon (equipped or not). LoadFromCSV
        // re-applies all crafted entries into the live catalogue and assigns
        // each an iLiveId, so all of these resolve through Get(). Used by the
        // between-round shop, which sells from the whole crafted collection.
        std::vector<int> AllCraftedLiveIds() const;

        // Ids the shop may sell: every loaded weapon (weapons_v2.csv catalogue
        // plus re-applied crafted) flagged shop_available. Evolution-only forms
        // set shop_available=0 so they only arrive by evolving.
        // iRound = the upcoming/current round; only weapons whose appearance
        // window [iMinRound, iMaxRound] contains it are returned (iMaxRound 0 =
        // no upper bound). Pass <= 0 to disable the round gate (all unlocked).
        std::vector<int> ShopWeaponIds(int iRound) const;

        // Cross-run persistence of the crafted registry.
        //   LoadCrafted — reads the saved registry into m_vecCrafted ONCE
        //     per process (guarded); later calls no-op so in-session crafts
        //     aren't clobbered. Call from scene init. Loaded entries get
        //     iLiveId = -1; LoadFromCSV's re-apply assigns live ids.
        //   SaveCrafted — writes the current registry. Call after every
        //     craft / equip change so the file is always current (this is
        //     more robust than saving only at exit, and avoids the
        //     separate-singleton-per-module issue if saved from the .exe).
        size_t LoadCrafted(const std::string& strPath);
        void   SaveCrafted(const std::string& strPath) const;

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
        // for the rest of the process run. Also saved to / loaded from disk
        // via SaveCrafted / LoadCrafted so they survive app restarts.
        std::vector<CraftedEntry>         m_vecCrafted;
        // LoadCrafted runs at most once per process (see its comment).
        bool                              m_bCraftedLoaded = false;
    };
}

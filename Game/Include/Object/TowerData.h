#pragma once

#include "../GameDefs.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Client
{
    // Which kind of tower a row describes. Attack towers fire their equipped
    // weapon; heal towers pulse HP to nearby allies. One row per kind today,
    // but the table is keyed by id so more rows can be added later.
    enum class TowerKind
    {
        Attack,
        Heal,
    };

    // Base stats for one placeable tower type, loaded from towers.csv. The
    // tower fires its equipped WEAPON, so these stats modify that weapon the
    // same way the player's stats do (fAttack = bullet-damage multiplier,
    // fAttackSpeed = fire-rate multiplier on the weapon cooldown, crit rolls
    // per shot). HP / defense / aggro / range drive the tower object itself.
    // Defaults mirror the GameDefs constants so a missing/short CSV still gives
    // the pre-feature behaviour.
    struct TowerDef
    {
        int         iId          = -1;
        std::string strName      = "Tower";
        TowerKind   eKind        = TowerKind::Attack;

        int   iHP          = kTowerHP;     // Attackable max HP
        float fAttack      = 1.f;          // bullet-damage multiplier (player-style)
        float fDefense     = 0.f;          // incoming-damage reduction (0..0.9)
        float fAttackSpeed = 1.f;          // fire-rate multiplier (>1 = faster)
        float fCritChance  = 0.f;          // 0..1 chance a shot crits
        float fCritMult    = 2.f;          // crit damage multiplier
        float fRange        = 12.f;        // attack targeting radius (cells)
        int   iPrice        = kTowerPrice; // shop buy cost
        int   iAggro        = kTowerAggro; // enemy targeting priority

        // Heal-tower only (ignored by attack towers).
        int   iHealAmount   = kHealAmount;
        float fHealInterval = kHealInterval;
        float fHealRadius   = kHealRadius;
    };

    // Static catalogue of tower types, loaded once from towers.csv at
    // scene-init time (GameScene::Init). Singleton, mirroring WeaponDatabase:
    // Tower / HealTower / the shop all read it without threading a pointer.
    class TowerDatabase
    {
    public:
        static TowerDatabase& GetInst()
        {
            static TowerDatabase inst;
            return inst;
        }

        // Parse towers.csv into the catalogue. Returns rows loaded; on failure
        // the table stays empty and every reader falls back to its TowerDef
        // defaults (the GameDefs constants).
        size_t LoadFromCSV(const std::string& strPath);

        // Look up by id; nullptr if unknown.
        const TowerDef* Get(int iId) const;
        // The first row of a given kind, or nullptr if none loaded. Tower /
        // HealTower use this since there is one row per kind today.
        const TowerDef* FirstOfKind(TowerKind eKind) const;

    private:
        TowerDatabase() = default;
        TowerDatabase(const TowerDatabase&) = delete;
        TowerDatabase& operator=(const TowerDatabase&) = delete;

        std::vector<TowerDef>           m_vecTowers;
        std::unordered_map<int, size_t> m_mapIdToIndex;
    };
}

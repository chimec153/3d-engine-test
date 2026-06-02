#pragma once

#include "../GameDefs.h"
#include "WeaponData.h"   // ImpactModule mask (reused for the tower's own effect)
#include <string>
#include <vector>
#include <unordered_map>

namespace Client
{
    // Which kind of tower a row describes. Only Attack (fires its equipped
    // weapon) and Heal (pulses HP to nearby allies) have behaviour wired up
    // today; the rest are DATA-ONLY placeholders for the designed tower set —
    // the loader preserves their kind so towers.csv reads faithfully, but no
    // gameplay code branches on them yet (the type-select UI + the slow / AOE /
    // pull / ally-buff behaviours are a later step). Tower::Init still consumes
    // only FirstOfKind(Attack) and HealTower only FirstOfKind(Heal), so adding
    // these rows leaves current gameplay unchanged.
    enum class TowerKind
    {
        Attack,   // gatling-style single-target turret (wired)
        Heal,     // HP regen aura          (wired)
        Frost,    // slow aura              (data-only)
        Mortar,   // lobbed AOE             (data-only)
        Gravity,  // pull / cluster         (data-only)
        Buff,     // amplify nearby towers  (data-only)
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

        // --- Designed-tower-set fields (loaded, not yet all consumed) ---------
        // Shop availability window by round. iLastRound == 0 means "no cap":
        // available from iFirstRound through the final round. (No consumer yet —
        // the shop does not gate on round; this is here for that future step.)
        int iFirstRound = 1;
        int iLastRound  = 0;

        // Which procedural body the tower should build — a semantic silhouette
        // key the future mesh-builder switches on. Two languages, both allowed:
        //   * composite recipe of CURRENT primitives (box / sphere / capsule),
        //     e.g. "post_orb" = square pillar + floating sphere. Buildable today
        //     with MeshPresets — no new generator — so this is the demo path.
        //   * regular N-gon prism "prism3".."prism8" (side-count = role at a
        //     glance). Cleaner but needs a future N-gon prism generator.
        // The builder owns the actual geometry; the CSV only names the look.
        // Tower::Init still hard-codes its AxisBox, so this field is unread now.
        std::string strMesh = "box";

        // Per-tower level-up (NEW per-tower-type model). A tower can be upgraded
        // up to iMaxLevel; each level past 1 ADDS these flat deltas (stackable):
        // HP, bullet-damage multiplier, and fire-rate multiplier. Role-specific
        // per-level effects (slow %, AOE radius, buff %) are future columns added
        // alongside those behaviours. No leveling flow exists yet — these load
        // but nothing applies them.
        int   iMaxLevel     = 1;
        int   iLvlHpAdd     = 0;
        float fLvlAtkAdd    = 0.f;
        float fLvlAtkSpdAdd = 0.f;

        // --- Intrinsic tower effect (layered onto the equipped weapon) --------
        // The tower applies this on-impact effect IN ADDITION to its weapon's
        // own effects, so an enemy struck by the tower's bullets gets the weapon
        // effects + this one (e.g. weapon = burn + knockback, tower = slow ->
        // enemy gets all three). uTowerImpact is an ImpactModule mask (one bit
        // in practice); fTowerEffectP0/P1 are that effect's params, per effect:
        //   Slow      = factor (0..1), duration (s)
        //   Gather    = pull (0..1),   radius (cells)
        //   Burn      = damage/tick,   duration (s)
        //   Knockback = force,         (P1 unused)
        // Impact_None = no tower effect. WIRED: Tower::FireAt / RespawnSustained
        // append it via Bullet::AddImpactEffect. (Beam-firing weapons don't use
        // Bullet impacts, so the tower effect doesn't ride laser weapons yet.)
        unsigned int uTowerImpact   = Impact_None;
        float        fTowerEffectP0 = 0.f;
        float        fTowerEffectP1 = 0.f;
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

        // All loaded rows, in CSV order. The shop iterates this to list the
        // buyable tower types (filtering by kind + round window itself).
        const std::vector<TowerDef>& All() const { return m_vecTowers; }

    private:
        TowerDatabase() = default;
        TowerDatabase(const TowerDatabase&) = delete;
        TowerDatabase& operator=(const TowerDatabase&) = delete;

        std::vector<TowerDef>           m_vecTowers;
        std::unordered_map<int, size_t> m_mapIdToIndex;
    };
}

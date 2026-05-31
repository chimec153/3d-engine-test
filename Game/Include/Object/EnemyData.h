#pragma once

#include <string>
#include <vector>

namespace Client
{
    // World-scale conversion. enemies.json/rounds.json are authored in
    // px/sec and px-radius units (top-down 2D spec); the 3D voxel world is
    // in cell units. Treat 50 px ≈ 1 cell so a crawler's 100 px/sec → 2.0
    // cells/sec (matches the legacy CSV-era speed). Centralised here so a
    // tuning pass can shift the conversion in one place.
    constexpr float kPxPerCell = 50.f;

    // Visual variant. JSON tier "basic" → Box, "elite"/"boss" → Capsule.
    // Both meshes share the same ApplyDef path; the only differences are
    // mesh + material colour.
    enum class EnemyKind
    {
        Box,
        Capsule,
        COUNT,
    };

    enum class EnemyTier
    {
        Basic,
        Elite,
        Boss,
    };

    // Boss-phase ability kind. Phase 3 introduced boss phase systems —
    // each boss phase ticks one (or two via alsoSummon) of these on its
    // own cooldown. None is the "no ability this phase" sentinel
    // (devourer phase 1 = pure chase, no extra ability).
    enum class AbilityType
    {
        None,
        Charge,    // dash with a telegraph; reuses TickDash plumbing
        Slam,      // telegraphed AoE in slamRadius
        Barrage,   // shotsPerVolley EnemyBullets spread over spreadDegrees
        Summon,    // spawn summonCount enemies of summonId
    };

    // One phase row from enemies.json bosses[].phases[]. hpThreshold is the
    // fraction of maxHP at which the phase activates — Enemy walks the
    // list every frame and the latest phase whose threshold is still
    // above the current HP fraction wins.
    struct BossPhase
    {
        float        fHpThreshold     = 1.f;
        float        fMoveSpeedMult   = 1.f;
        std::string  strName;                  // debug only
        AbilityType  eAbility         = AbilityType::None;
        float        fAbilityCooldown = 0.f;
        float        fTelegraphTime   = 0.f;
        // Charge ability (devourer phase 2)
        float        fChargeSpeedPx   = 0.f;
        // Slam ability (devourer phase 3)
        float        fSlamRadiusPx    = 0.f;
        int          iSlamDamage      = 0;
        // Barrage ability (hive_queen phases 1, 3)
        float        fProjSpeedPx     = 0.f;
        int          iProjDamage      = 0;
        int          iShotsPerVolley  = 0;
        float        fSpreadDegrees   = 360.f;
        // Summon ability (hive_queen phase 2)
        std::string  strSummonId;
        int          iSummonCount     = 0;
        // alsoSummon — a second summon channel that runs alongside the
        // primary ability on its own cooldown. hive_queen phase 3 uses
        // this to spawn swarmlings while the barrage fires.
        std::string  strAltSummonId;
        int          iAltSummonCount  = 0;
        float        fAltSummonCooldown = 0.f;
    };

    // One enemy archetype from enemies.json. Mixed base + derived fields:
    //  - iBaseHp / iContactDamage / fMoveSpeedPx / fHitboxRadiusPx are the
    //    raw values straight out of JSON.
    //  - iMaxHP / iAttackMin / iAttackMax / fSpeed are *derived* — the
    //    loader seeds them from the base values (no multipliers), and
    //    EnemySpawner makes a per-spawn copy that bakes the current
    //    round's hpMultiplier / damageMultiplier in before calling
    //    Enemy::ApplyDef. Keeping the consumer (Enemy) unaware of the
    //    multiplier maths means nothing changes on the Enemy side as we
    //    add more multipliers in Phase 2 (boss aura, debuff zones).
    struct EnemyDef
    {
        // --- identity ---
        int          iId             = 0;        // dense 1..N index assigned by load order
        std::string  strIdKey;                   // "crawler" / "swarmling" — round spawns reference this
        std::string  strName;                    // display label
        std::string  strBehavior     = "chase";  // chase | dash | ranged_kite | ... Phase 1: only chase honored
        EnemyTier    eTier           = EnemyTier::Basic;
        EnemyKind    eKind           = EnemyKind::Box;

        // --- base stats (raw JSON, no multipliers) ---
        int          iBaseHp         = 10;
        int          iContactDamage  = 5;
        float        fMoveSpeedPx    = 100.f;
        float        fHitboxRadiusPx = 16.f;
        int          iGoldReward     = 1;
        int          iXpReward       = 1;

        // --- visual ---
        unsigned int uColorRGB       = 0xFF1A1A;

        // --- derived runtime stats (loader seeds; spawner mutates a copy with round multipliers) ---
        int          iMaxHP          = 10;
        int          iAttackMin      = 1;
        int          iAttackMax      = 2;
        float        fSpeed          = 2.f;      // cells/sec
        float        fAttackRange    = 1.5f;     // world units
        float        fAttackCooldown = 1.0f;     // seconds between melee hits

        // --- behavior special params (Phase 2). 0 means "not present".
        //     Each block corresponds to one JSON `special.type`. EnemyDef
        //     carries all blocks flat so Enemy can branch on m_strBehavior
        //     + presence of a non-zero param block without a polymorphic
        //     special hierarchy. Px-suffixed fields use kPxPerCell at use
        //     site (e.g. fDashSpeed = fDashSpeedPx / kPxPerCell).
        // dash (behavior == "dash"):
        float        fDashSpeedPx        = 0.f;  // px/sec; converted at use
        float        fDashCooldown       = 0.f;  // seconds
        float        fDashRangePx        = 0.f;  // px — both trigger AND distance
        float        fDashTelegraph      = 0.f;  // seconds of pause-and-warn before the dash

        // ranged (special.type == "ranged"; behavior usually "ranged_kite"):
        float        fProjSpeedPx        = 0.f;
        int          iProjDamage         = 0;
        float        fFireCooldown       = 0.f;
        float        fPreferredRangePx   = 0.f;  // px the kiter tries to keep from the player

        // explode (special.type == "explode"; behavior usually "chase"):
        float        fExplodeRadiusPx    = 0.f;
        int          iExplodeDamage      = 0;
        float        fFuseTime           = 0.f;
        float        fTriggerRangePx     = 0.f;  // distance to player that lights the fuse

        // --- Phase 3 specials ---
        // split (splitter; special.type == "split"). On death, spawn
        // iSplitCount enemies of strSplitId in a small ring around the
        // corpse before the orb drops.
        std::string  strSplitId;
        int          iSplitCount         = 0;

        // shield (shieldbearer; special.type == "shield"). Damage incoming
        // from inside the front-facing arc (centred on movement direction)
        // is multiplied by (1 - fShieldReduction).
        float        fShieldArcDegrees   = 0.f;
        float        fShieldReduction    = 0.f;   // 0..1

        // summon (summoner; special.type == "summon"). Periodically spawn
        // iSummonCount enemies of strSummonId at the summoner. Movement is
        // ranged_kite (uses fPreferredRangePx above for distance keeping).
        std::string  strSummonId;
        int          iSummonCount        = 0;
        float        fSummonCooldown     = 0.f;

        // blink (phantom; special.type == "blink"). Periodically teleport
        // forward (toward player) by fBlinkDistancePx.
        float        fBlinkCooldown      = 0.f;
        float        fBlinkDistancePx    = 0.f;

        // --- Boss flag + phase list ---
        // Set true for entries from enemies.json bosses[] (not enemies[]).
        // Spawner sizes the body bigger, materialises only one per round,
        // and the phase list below drives phase transitions on HP
        // threshold crossings.
        bool                    bIsBoss        = false;
        int                     iAppearsRound  = 0;
        std::vector<BossPhase>  vecPhases;
    };
}

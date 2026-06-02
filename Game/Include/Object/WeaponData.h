#pragma once

#include <cmath>
#include <string>

namespace Client
{
    // Weapon system data model — all enum values are also the strings
    // accepted in weapons.csv (case-insensitive parsing in WeaponDatabase).

    enum class SpawnOrigin
    {
        Front,      // A small offset directly in front of the player.
        Around,     // The player's own position (radial / orbital weapons).
        Mouse,      // Mouse-cursor world position on the player's y-plane.
        Random,     // A random point in a ring around the player (y-plane).
        COUNT,
    };

    enum class MovementType
    {
        Straight,   // Constant forward velocity (the legacy Bullet behaviour).
        Spiral,     // Forward + perpendicular sine sway over time.
        Fixed,      // Sits where it spawned until lifetime expires.
        Orbital,    // Circles the owner (player) at a constant radius.
        Homing,     // Steers toward the nearest enemy each frame, then flies.
        Aimed,      // Locks onto the nearest enemy once at spawn, then straight.
        Follow,     // Not a projectile path: marks a "pet" weapon. Player spawns
                    // a companion entity (Pet) that trails the player and fires
                    // on its own cooldown. Projectiles never use this value (the
                    // pet fires Straight bullets); MakeBulletMovement's default
                    // covers it if one ever does.
        COUNT,
    };

    // How an auto-targeting projectile (Aimed / Homing movement) chooses which
    // enemy to lock onto. Ignored by every other movement type -- those fly
    // along the spawn heading (player facing / mouse). Nearest is the legacy
    // default. The integer values are what weapons.csv / parts.csv store.
    enum class AimMode
    {
        Nearest,    // Closest active enemy (legacy behaviour).
        LowestHP,   // Active enemy with the least current HP (finisher / anti-summoner).
        Random,     // A uniformly random active enemy.
        Forward,    // Fly along the player's facing / move direction.
        Cursor,     // Aim at the mouse cursor's world point on the player plane.
        Radial,     // Split `count` projectiles evenly around 360 degrees.
        COUNT,
    };

    // True for the enemy-seeking modes (a FindTargetEnemy lookup picks the
    // heading); false for the geometric modes (Forward / Cursor / Radial) that
    // derive the heading from the player, cursor, or an even ring.
    inline bool AimSeeksEnemy(AimMode e)
    {
        return e == AimMode::Nearest || e == AimMode::LowestHP || e == AimMode::Random;
    }

    enum class FireMode
    {
        Cooldown,   // Re-spawn a projectile every `cooldown` seconds.
        Sustained,  // Spawn once when the weapon is gained; the instance
                    // lives forever (Orbital + Sustained = shield orb).
        COUNT,
    };

    enum class OnHitEvent
    {
        Vanish,     // Despawn on first hit (default Bullet behaviour).
        NoChange,   // Keep flying, ignore the hit. Used by orbital / piercing.
        Reflect,    // Flip forward direction (and zero the distance counter
                    // so the lifetime guard doesn't insta-kill it).
        Multiply,   // Despawn this one and spawn two fanned-out children.
        Field,      // Damage-over-time zone: never despawns on contact and
                    // ticks damage to enemies standing in it (Enemy applies
                    // it on collision STAY). Pair with Fixed movement + a
                    // long lifetime + big size for a "장판".
        Chain,      // On hit, redirect at the nearest not-yet-hit enemy and
                    // keep flying; max_hits caps the number of links. Distinct
                    // from Reflect (wall bounce) and Multiply (spawns children).
        COUNT,
    };

    enum class ProjectileShape
    {
        Sphere,     // Uses Engine::Sphere helpers (the legacy Bullet mesh).
        Box,        // Engine::Box::CreateTextureVertex unit cube.
        Triangle,   // Procedural flat triangle (3 verts, single face).
        COUNT,
    };

    // Per-weapon tracer-trail look. Each value is a preset bundling the
    // ribbon's length / width / brightness / head-glow, resolved by
    // TrailRenderManager::GetPreset. The bullet colour still tints it; only
    // the shape/intensity changes. None disables the trail (beams / orbitals /
    // fixed fields). The strings here are what weapons_v2.csv stores.
    enum class TrailStyle
    {
        None,       // No trail.
        Tracer,     // Default: thin, bright, short-to-medium streak + head glow.
        Plasma,     // Thick, very bright, long — energy weapons.
        Spark,      // Short, thin, snappy — rapid pellets.
        Comet,      // Long, wide-tapering fading tail.
        COUNT,
    };

    // Which stat the level-up bump touches. The amount column means
    // different things per field: Damage / Count are additive integers,
    // Cooldown is a multiplier (e.g. 0.9 = -10%/level), Speed and Size are
    // additive (world-units/sec and uniform scale respectively).
    enum class LevelUpField
    {
        Damage,
        Cooldown,
        Count,
        Speed,
        Size,
        COUNT_,    // suffixed because COUNT collides with the enum name on
                   // older MSVC parses inside macro contexts
    };

    // Impact modules attached to a weapon. Bit flags so a single weapon can
    // stack several (Damage + Knockback + Gather). Damage is the always-on
    // baseline (the existing per-hit damage applied by Enemy on collision);
    // Knockback / Gather are additive effects the bullet applies on impact
    // via the IImpactEffect strategies. The variant column in parts.csv is
    // the bit index, so a part's flag is (1u << variant).
    enum ImpactModule : unsigned int
    {
        Impact_None      = 0,
        Impact_Damage    = 1u << 0,   // baseline per-hit damage (always set)
        Impact_Knockback = 1u << 1,   // shove the struck enemy away from impact
        Impact_Gather    = 1u << 2,   // pull nearby enemies toward the impact
        Impact_Burn      = 1u << 3,   // apply a damage-over-time burn status
        Impact_Slow      = 1u << 4,   // slow the struck enemy's movement
    };

    // Number of impact module types — also VariantCount(CAT_IMPACT) in the
    // combiner, which validates the parts.csv variant column.
    constexpr int kImpactModuleCount = 5;

    struct WeaponDef
    {
        int             iId = -1;
        std::string     strName;

        SpawnOrigin     eOrigin   = SpawnOrigin::Front;
        MovementType    eMovement = MovementType::Straight;
        FireMode        eFireMode = FireMode::Cooldown;
        OnHitEvent      eOnHit    = OnHitEvent::Vanish;
        ProjectileShape eShape    = ProjectileShape::Sphere;

        // Tracer-trail preset. Defaults to Tracer so weapons without the
        // column keep the standard streak; None disables it. See TrailStyle.
        TrailStyle      eTrailStyle = TrailStyle::Tracer;

        // Aim / heading mode. Drives the spawn heading for every weapon
        // (Forward = player facing, the legacy default; Cursor = mouse; Radial
        // = even ring) and the target the Aimed/Homing movers seek
        // (Nearest/LowestHP/Random). Defaults to Forward so weapons that don't
        // opt in keep firing along the player's facing as before.
        AimMode         eAimMode  = AimMode::Forward;

        // Max enemies a projectile may hit before it despawns. 0 = unlimited
        // (the on-hit behaviour decides: Vanish dies on the 1st hit, NoChange
        // pierces forever, etc.). >0 caps piercing/bouncing projectiles to
        // that many hits. Field zones ignore it (they're duration-based).
        int   iMaxHits         = 0;

        // Damage-over-time tick interval, in seconds, for persistent weapons.
        // 0 = a single instantaneous hit on contact (normal projectiles). >0 =
        // the weapon applies its damage to overlapping enemies once every
        // fDamageInterval seconds (Enemy ticks it on collision STAY) -- used by
        // Sustained orbits / auras / beams and by Field zones. Lower = faster
        // ticks = higher sustained DPS, the per-card balance lever for these.
        // A Field weapon with 0 here falls back to the legacy 0.5s tick.
        float fDamageInterval  = 0.f;

        int   iDamage          = 5;
        float fCooldown        = 0.5f;   // seconds (ignored for Sustained)
        float fProjectileSpeed = 8.f;    // world units / sec
        float fLifetime        = 2.f;    // seconds; Fixed/Orbital cap too
        int   iCount           = 1;      // projectiles per fire
        // Visual scale of the bullet mesh. Collider radius scales
        // proportionally so a bigger projectile both *looks* bigger
        // and *hits* over a wider area. Default matches the legacy
        // 0.25 uniform scale Bullet::Init used to hard-code.
        float fSize            = 0.25f;
        // Speed delta per second. Bullet::Update applies
        //   speed += acceleration * dt
        // each frame, so positive values accelerate the projectile and
        // negative values decelerate it. 0 keeps the legacy constant-
        // speed behaviour. Orbital weapons interpret fProjectileSpeed
        // as angular rad/sec, so acceleration is angular too.
        float fAcceleration    = 0.0f;

        // Fan spread for a Count>1 burst, in DEGREES of total arc the shots
        // are distributed across (centred on the aim heading). < 0 (the
        // default) keeps the legacy ~10 deg-per-shot fan; 0 stacks them into a
        // single line; > 0 spreads Count shots evenly over that arc. Ignored
        // by Radial aim (even 360 ring) and by Sustained weapons.
        float fSpreadDeg       = -1.f;

        // Orbital-only: radius (world units) the projectile circles the
        // owner at. 0 = sits centred on the player and follows them (a
        // player-following zone). Ignored by non-orbital movement. The
        // legacy default reproduces the old hard-coded 0.9 orbit.
        float fOrbitRadius     = 0.9f;

        // Orbital-only: radial growth in world units/sec. >0 spirals the
        // orbit outward — R(t) = fOrbitRadius + fRadialSpeed*t. 0 keeps a
        // fixed-radius orbit (the legacy behaviour). The orb despawns once the
        // radius passes kMaxOrbitRadius (OrbitalMovement::WantsDespawn), so a
        // Sustained spiral can't grow without bound. Ignored by non-orbital
        // movement.
        float fRadialSpeed     = 0.f;

        // Random-origin only: the ring radius (world/cell units) the projectile
        // rains down within around the player. Spawn distance is random in
        // [fSpawnRadius/3, fSpawnRadius], so the legacy default 6 reproduces the
        // old hard-coded 2..6 ring. Ignored by every other SpawnOrigin.
        float fSpawnRadius     = 6.f;

        // 0xRRGGBB packed. Used for the card colour and (for now) the
        // projectile material tint — no per-weapon texture pipeline yet.
        unsigned int uColorRGB = 0xFFFFFF;

        // Per-level bump for EACH stat, indexed by LevelUpField (0 = that stat
        // doesn't grow on level-up). A weapon can now level several stats at
        // once (the combiner's level-up category is multi-select). Cooldown is
        // a multiplier (e.g. 0.9 = -10%/level; 0 = unchanged); Damage / Count
        // are additive ints; Speed / Size are additive floats.
        float fLevelUpAmt[static_cast<size_t>(LevelUpField::COUNT_)] = {};

        // Impact modules (bit flags from ImpactModule). Damage is the baseline
        // and is always present; Knockback / Gather are optional add-ons the
        // bullet runs on impact (built into IImpactEffect strategies by
        // MakeImpactEffects). A weapon can stack several distinct modules.
        unsigned int uImpactMask = Impact_Damage;
        // Knockback impulse magnitude (world units/sec) applied to the struck
        // enemy, directed away from the impact point. Used when Knockback set.
        float fKnockback    = 6.f;
        // Gather pull fraction (0..1, clamped): how far toward the impact point
        // every enemy within fGatherRadius is dragged — 1 = onto the centre.
        // Used when Gather is set.
        float fGatherPull   = 1.f;
        // Gather AoE radius (world units) — enemies inside it are pulled toward
        // the impact point. Used when Gather is set.
        float fGatherRadius = 4.f;
        // Burn damage-over-time: damage applied per tick to a burning enemy.
        // Used when Burn is set. The tick interval lives on Enemy.
        int   iBurnDamage   = 3;
        // Burn duration (seconds) — how long the burn status lasts after a hit
        // (re-hitting refreshes to the longer remaining time). Used when Burn set.
        float fBurnDuration = 3.f;
        // Slow movement multiplier applied to a slowed enemy's chase speed
        // (0..1; lower = slower). Used when Slow is set.
        float fSlowFactor   = 0.5f;
        // Slow duration (seconds) — re-applying refreshes to the longer
        // remaining time and the stronger (lower) factor. Used when Slow set.
        float fSlowDuration = 2.5f;

        // Evolution (v2). At weapon level >= iEvolveMinLevel the slot
        // transforms into weapon id iEvolvesInto (0 = never evolves). A
        // one-time upgrade -- the evolved form is the final shape. bShopAvailable
        // gates whether the shop offers this weapon (evolution-only forms set
        // it false so they only appear via evolving). Defaults: no evolution,
        // shop-available (so legacy rows still show up).
        int  iEvolvesInto    = 0;
        int  iEvolveMinLevel = 0;
        bool bShopAvailable  = true;

        // Per-weapon shop price (weapons_v2.csv col 26). 0 / blank / missing =
        // fall back to the global kWeaponPrice default. Drives buy, merge, and
        // sell-refund in the intermission shop, so a weapon can be made cheaper
        // or pricier than the baseline individually.
        int  iPrice          = 0;

        // Appearance window in the shop / start pick (weapons_v2.csv cols 27-28).
        // iMinRound: first round it can appear (0/1 = from the start). iMaxRound:
        // last round it appears (0 = no cap, available for the rest of the run).
        // The shop filters by the upcoming round: a weapon shows while
        // iMinRound <= round and (iMaxRound == 0 || round <= iMaxRound).
        int  iMinRound       = 0;
        int  iMaxRound       = 0;
    };

    // Hard cap on a spiralling-orbit radius (used when fRadialSpeed > 0). Once
    // the growing radius passes this, OrbitalMovement::WantsDespawn returns
    // true and Bullet::Update despawns the projectile, so a Sustained spiral
    // can't grow without bound.
    constexpr float kMaxOrbitRadius = 200.f;

    // Hard cap on a weapon slot's level. LevelUpSlot won't raise iLevel past
    // this — evolution resets to 1, so the evolved form gets its own 1..cap
    // range. Stat readers below are unaffected (they just see a clamped level).
    constexpr int kMaxWeaponLevel = 10;

    // Level-aware stat lookups. Centralised here so Bullet (damage / speed)
    // and Player slots (cooldown / count) read the same formulas — each weapon
    // can level any subset of the stats (fLevelUpAmt[field] != 0).
    // Per-field level-up amount (0 = that stat doesn't grow). Helper so the
    // Compute* readers stay terse.
    inline float LevelUpAmt(const WeaponDef& def, LevelUpField f)
    {
        return def.fLevelUpAmt[static_cast<size_t>(f)];
    }
    inline int ComputeDamage(const WeaponDef& def, int iLevel)
    {
        int d = def.iDamage;
        d += static_cast<int>(LevelUpAmt(def, LevelUpField::Damage) * static_cast<float>(iLevel - 1));
        return d;
    }
    inline float ComputeCooldown(const WeaponDef& def, int iLevel)
    {
        float c = def.fCooldown;
        const float m = LevelUpAmt(def, LevelUpField::Cooldown);
        if (m > 0.f)   // multiplier; 0 = "not a level-up stat"
            c *= std::pow(m, static_cast<float>(iLevel - 1));
        // Defensive floor — a runaway negative cooldown would spawn a
        // bullet every frame.
        return c < 0.05f ? 0.05f : c;
    }
    inline int ComputeCount(const WeaponDef& def, int iLevel)
    {
        int n = def.iCount;
        n += static_cast<int>(LevelUpAmt(def, LevelUpField::Count) * static_cast<float>(iLevel - 1));
        return n < 1 ? 1 : n;
    }
    inline float ComputeSpeed(const WeaponDef& def, int iLevel)
    {
        float s = def.fProjectileSpeed;
        s += LevelUpAmt(def, LevelUpField::Speed) * static_cast<float>(iLevel - 1);
        return s;
    }
    inline float ComputeSize(const WeaponDef& def, int iLevel)
    {
        float sz = def.fSize;
        sz += LevelUpAmt(def, LevelUpField::Size) * static_cast<float>(iLevel - 1);
        // Defensive floor — a zero/negative scale would collapse the mesh
        // and the derived collider radius.
        return sz < 0.01f ? 0.01f : sz;
    }

    // Multiply split generations for a build: how many times it can split
    // before children just despawn on hit. Driven by iMaxHits (min 1 so it
    // always splits once); capped so 2^depth can't explode into hundreds of
    // bullets. 0 for non-Multiply weapons.
    inline int MultiplySplitDepth(const WeaponDef& def)
    {
        if (def.eOnHit != OnHitEvent::Multiply) return 0;
        const int d = (def.iMaxHits <= 0) ? 1 : def.iMaxHits;
        return d > 10 ? 10 : d;
    }

    // Heuristic "power score" for a build — a relative gauge shown in the
    // combo scene, not a balance guarantee. Adapts the design-doc skeleton
    // to the engine's WeaponDef: the on-hit behaviour drives the
    // *multiplicative* factors (pierce / bounce / multiply), size the AoE
    // footprint; base DPS and coverage are the additive-ish parts. The
    // multiplicative block is where "괴랄한" builds come from — several
    // multipliers stacking blows the score up fast, which is exactly what
    // makes an outlier easy to spot.
    // iLevel folds the per-level bump in (via the Compute* helpers), so the
    // chosen LevelUp field actually moves the score — call it at level 1 for
    // the current score and level 2 for "score after one level-up".
    inline float CalcPowerScore(const WeaponDef& def, int iLevel = 1)
    {
        // Level-aware stats — the LevelUp field's bump is applied here, so a
        // Damage/Cooldown/Count/Speed/Size level-up each raises the score.
        const float fDamage   = static_cast<float>(ComputeDamage(def, iLevel));
        const float fCooldown = ComputeCooldown(def, iLevel);
        const float fSpeed    = ComputeSpeed(def, iLevel);
        const float fSize     = ComputeSize(def, iLevel);
        const int   iCount    = ComputeCount(def, iLevel);

        // Base DPS = damage × fire rate. Sustained weapons hit on contact
        // rather than on a cooldown, so use a nominal continuous rate.
        const float fFireRate =
            (def.eFireMode == FireMode::Cooldown && fCooldown > 0.01f)
                ? 1.0f / fCooldown
                : 4.0f;
        const float fBaseDPS = fDamage * fFireRate;

        // Multiplicative factors — the explosive part.
        // Pierce ≈ enemies struck. NoChange pierces up to iMaxHits (or "many"
        // when uncapped); Field blankets a zone; Multiply doesn't pierce (it
        // splits — counted by fMultiply below). iMaxHits scales the NoChange
        // case so a capped pierce scores below an unlimited one.
        float fPierce = 0.0f;
        if      (def.eOnHit == OnHitEvent::Field)    fPierce = 3.0f;
        else if (def.eOnHit == OnHitEvent::NoChange) fPierce = (def.iMaxHits > 0) ? static_cast<float>(def.iMaxHits) : 4.0f;
        const float fBounce   = (def.eOnHit == OnHitEvent::Reflect)  ? 1.0f : 0.0f;
        // Multiply cascades as a binary tree: a depth-d build spawns
        //   1 + 2 + 4 + ... + 2^d  ==  2^(d+1) - 1
        // bullets, each landing one hit's damage. The factor is that total
        // hit count (the old linear 1+depth ignored the doubling its own
        // comment claimed). It's the ceiling — each generation needs a fresh
        // enemy to land on — but the score gauges build *potential*, so the
        // explosive ceiling is exactly what we want to surface.
        const int   iSplitDepth = MultiplySplitDepth(def);
        const float fMultiply   = (def.eOnHit == OnHitEvent::Multiply)
            ? static_cast<float>((1 << (iSplitDepth + 1)) - 1) : 1.0f;
        const float fAoe      = fSize;               // collider radius scales with size
        const float fCritChance = 0.2f, fCritMult = 2.0f;  // engine's provisional global crit

        float fMult = 1.0f;
        fMult *= (1.0f + fPierce * 0.5f);
        fMult *= (1.0f + fBounce * 0.3f);
        fMult *= (1.0f + fAoe    * 0.4f);
        fMult *= fMultiply;
        fMult *= fCritMult * fCritChance + (1.0f - fCritChance);

        // Coverage = reach × projectile count, sqrt-dampened. Reach is
        // bounded per movement so a 9999s Sustained lifetime can't blow up.
        float fReach;
        switch (def.eMovement)
        {
        case MovementType::Orbital:
        {
            // Spiral-out covers a growing radius — gauge by the mid radius
            // between the start and the (lifetime- or cap-limited) end.
            float fEndR = def.fOrbitRadius;
            if (def.fRadialSpeed > 0.f)
            {
                fEndR = def.fOrbitRadius + def.fRadialSpeed * def.fLifetime;
                if (fEndR > kMaxOrbitRadius)
                {
                    fEndR = kMaxOrbitRadius;
                }
            }
            fReach = fmaxf(0.5f, (def.fOrbitRadius + fEndR));   // mid radius * 2
            break;
        }
        case MovementType::Fixed:   fReach = fmaxf(0.5f, fSize);                   break;
        default:                    fReach = fmaxf(0.5f, fSpeed * fminf(def.fLifetime, 3.0f)); break;
        }
        const float fCoverage = fReach * static_cast<float>(iCount);

        return fBaseDPS * fMult * sqrtf(fCoverage);
    }
}

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
        COUNT,
    };

    enum class MovementType
    {
        Straight,   // Constant forward velocity (the legacy Bullet behaviour).
        Spiral,     // Forward + perpendicular sine sway over time.
        Fixed,      // Sits where it spawned until lifetime expires.
        Orbital,    // Circles the owner (player) at a constant radius.
        COUNT,
    };

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
        COUNT,
    };

    enum class ProjectileShape
    {
        Sphere,     // Uses Engine::Sphere helpers (the legacy Bullet mesh).
        Box,        // Engine::Box::CreateTextureVertex unit cube.
        Triangle,   // Procedural flat triangle (3 verts, single face).
        COUNT,
    };

    // Which stat the level-up bump touches. The amount column means
    // different things per field: Damage / Count are additive integers,
    // Cooldown is a multiplier (e.g. 0.9 = -10%/level), Speed is additive
    // world-units/sec.
    enum class LevelUpField
    {
        Damage,
        Cooldown,
        Count,
        Speed,
        COUNT_,    // suffixed because COUNT collides with the enum name on
                   // older MSVC parses inside macro contexts
    };

    struct WeaponDef
    {
        int             iId = -1;
        std::string     strName;

        SpawnOrigin     eOrigin   = SpawnOrigin::Front;
        MovementType    eMovement = MovementType::Straight;
        FireMode        eFireMode = FireMode::Cooldown;
        OnHitEvent      eOnHit    = OnHitEvent::Vanish;
        ProjectileShape eShape    = ProjectileShape::Sphere;

        int   iDamage          = 5;
        float fCooldown        = 0.5f;   // seconds (ignored for Sustained)
        float fProjectileSpeed = 8.f;    // world units / sec
        float fLifetime        = 2.f;    // seconds; Fixed/Orbital cap too
        int   iCount           = 1;      // projectiles per fire

        // 0xRRGGBB packed. Used for the card colour and (for now) the
        // projectile material tint — no per-weapon texture pipeline yet.
        unsigned int uColorRGB = 0xFFFFFF;

        LevelUpField eLevelUpField = LevelUpField::Damage;
        float        fLevelUpAmount = 1.f;
    };

    // Level-aware stat lookups. Centralised here so Bullet (damage / speed)
    // and Player slots (cooldown / count) read the same formulas — the
    // level-up CSV column only touches one field per weapon, the other
    // three pass through untouched.
    inline int ComputeDamage(const WeaponDef& def, int iLevel)
    {
        int d = def.iDamage;
        if (def.eLevelUpField == LevelUpField::Damage)
            d += static_cast<int>(def.fLevelUpAmount * static_cast<float>(iLevel - 1));
        return d;
    }
    inline float ComputeCooldown(const WeaponDef& def, int iLevel)
    {
        float c = def.fCooldown;
        if (def.eLevelUpField == LevelUpField::Cooldown)
            c *= std::pow(def.fLevelUpAmount, static_cast<float>(iLevel - 1));
        // Defensive floor — a runaway negative cooldown would spawn a
        // bullet every frame.
        return c < 0.05f ? 0.05f : c;
    }
    inline int ComputeCount(const WeaponDef& def, int iLevel)
    {
        int n = def.iCount;
        if (def.eLevelUpField == LevelUpField::Count)
            n += static_cast<int>(def.fLevelUpAmount * static_cast<float>(iLevel - 1));
        return n < 1 ? 1 : n;
    }
    inline float ComputeSpeed(const WeaponDef& def, int iLevel)
    {
        float s = def.fProjectileSpeed;
        if (def.eLevelUpField == LevelUpField::Speed)
            s += def.fLevelUpAmount * static_cast<float>(iLevel - 1);
        return s;
    }
}

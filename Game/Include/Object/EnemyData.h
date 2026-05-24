#pragma once

#include <string>

namespace Client
{
    // Enemy variant. Both visual mesh and the per-kind material colour
    // are picked from this — EnemyDatabase rows are keyed by id, and the
    // kind column tells Enemy which mesh + material to bind.
    enum class EnemyKind
    {
        Box,
        Capsule,
        COUNT,
    };

    struct EnemyDef
    {
        int          iId             = 0;        // matches the id column
        std::string  strName;                    // debug label only
        EnemyKind    eKind           = EnemyKind::Box;
        int          iMaxHP          = 10;
        float        fSpeed          = 2.0f;     // cells / sec
        float        fAttackRange    = 1.5f;     // world units
        float        fAttackCooldown = 1.0f;     // seconds between hits

        // Diffuse colour (0xRRGGBB packed). One Material is cached per
        // id so a row with a new colour gets its own material; multiple
        // rows sharing a colour can share a material via the id key.
        unsigned int uColorRGB       = 0xFF1A1A; // default red (legacy Box tint)

        // Melee damage range. Attackable picks a uniform value in
        // [iAttackMin, iAttackMax] per hit. Defaults match the old
        // hard-coded (1, 2) range Enemy::Init used.
        int          iAttackMin      = 1;
        int          iAttackMax      = 2;
    };
}

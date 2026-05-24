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
    };
}

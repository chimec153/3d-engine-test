#pragma once

#define INVENTORY_WIDTH  8
#define INVENTORY_HEIGHT 4

enum class WEAPON_TYPE
{
    FIST,
    SWORD,
    GUN,
    END
};

namespace Client
{
    // Single-layer wall convention. The voxel world only ever has solid
    // blocks at the floor (y=0) and walls (y=kWallY). Player movement,
    // pathfinder, and build/break all assume blocks live at this Y.
    constexpr int kWallY = 1;
}

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

    // Vertical offset from the Player pivot down to projectile-muzzle
    // height. The Player pivot sits at kWallY+1 (≈2.0) but enemy
    // collider centres sit at kWallY+0.3 (≈1.3) — bullets spawned at
    // the pivot would fly over enemy heads. Cooldown spawn position,
    // Mouse-mode raycast lift, and Orbital Bullet::Update all add this
    // so projectile altitudes line up with enemy hitboxes.
    constexpr float kMuzzleYOffset = -0.7f;
}

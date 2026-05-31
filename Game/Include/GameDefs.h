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

    // Economy: money granted per orb pickup, and the flat price to unlock a
    // new weapon in the between-round shop. Tunable — orbs drop one per enemy
    // kill, so a round of N enemies earns N * kOrbMoney.
    constexpr int kOrbMoney    = 10;
    constexpr int kWeaponPrice = 50;
    constexpr int kTowerPrice  = 30;   // cost to buy one more placeable tower

    // Aggro + tower durability. Enemies path to and attack the highest-aggro
    // active target; towers out-aggro the player so they form the front line.
    constexpr int kPlayerAggro = 1;
    constexpr int kTowerAggro  = 3;
    constexpr int kTowerHP     = 100;  // a tower breaks when its HP hits 0

    // Heal tower — periodically restores HP to nearby allies (player + towers).
    constexpr int   kHealTowerPrice = 40;
    constexpr int   kHealTowerHP    = 80;
    constexpr int   kHealAmount     = 15;    // HP restored per pulse
    constexpr float kHealRadius     = 6.f;   // world units
    constexpr float kHealInterval   = 3.f;   // seconds between pulses
}

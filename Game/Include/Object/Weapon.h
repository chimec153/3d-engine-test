#pragma once

#include <memory>
#include <vector>

namespace Client
{
    class Bullet;
    class Pet;
    class Beam;

    // A single owned weapon INSTANCE, managed by shared_ptr. Exactly one holder
    // points to it at a time — a player equip slot, the player inventory, or a
    // tower — so "where the pointer lives" IS the weapon's location (no
    // ownership flags / copy-counting needed). Carries the weapon's own LEVEL
    // (independent of a tower's level) plus the holder-driven firing state and
    // live persistent instances (Orbital orbs / Follow pets / laser Beams).
    //
    // iWeaponId is the WeaponDatabase definition id (what the weapon IS); the
    // level scales its damage / count / cooldown. A tower that holds this weapon
    // fires it at THIS level and layers its own tower-level bonuses on top.
    struct Weapon
    {
        int   iWeaponId    = -1;   // WeaponDatabase def id
        int   iLevel       = 1;    // weapon level (own scaling)
        float fCooldownAcc = 0.f;  // Cooldown-mode firing accumulator (current holder)
        // Live persistent instances the current holder spawns/drives. Empty
        // while the weapon sits idle (inventory) or its holder uses its own
        // tracking (a tower keeps these on the tower side, not here).
        std::vector<std::weak_ptr<Bullet>> vecSustainedInstances;
        std::vector<std::weak_ptr<Pet>>    vecPets;
        std::vector<std::weak_ptr<Beam>>   vecBeams;
        bool  bInstancesLive = false;
        // Player-side location: equipped (firing) vs inventory (idle). Only
        // meaningful while a PLAYER holds it; ignored once moved onto a tower.
        bool  bEquipped = true;

        Weapon() = default;
        Weapon(int iId, int iLv) : iWeaponId(iId), iLevel(iLv) {}
    };

    using WeaponPtr = std::shared_ptr<Weapon>;
}

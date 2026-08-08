#pragma once

#include "../Object/WeaponData.h"
#include "../Object/WeaponDatabase.h"
#include "../Object/TowerData.h"
#include <string>

// Shared hover-tooltip TEXT builders for the in-game HUDs (weapon HUD, tower
// HUD). Header-only inline so both HUD translation units share one copy (the
// between-round shop keeps its own richer builders). Stats are computed from the
// public WeaponData.h helpers + the def tables, so no engine state is needed.
namespace Client { namespace HudTip
{
    inline std::wstring ToW(const std::string& s) { return std::wstring(s.begin(), s.end()); }

    // One-decimal float → wstring without <sstream> (e.g. 1.5).
    inline std::wstring F1(float v)
    {
        const bool neg = v < 0.f;
        const int  t   = static_cast<int>((neg ? -v : v) * 10.f + 0.5f);
        return (neg ? std::wstring(L"-") : std::wstring()) +
               std::to_wstring(t / 10) + L"." + std::to_wstring(t % 10);
    }

    // Weapon stats at a given level (damage / count / fire mode / speed / size).
    inline std::wstring Weapon(const WeaponDef& def, int iLevel)
    {
        if (iLevel < 1) iLevel = 1;
        const int   dmg = ComputeDamage(def, iLevel);
        const int   cnt = ComputeCount (def, iLevel);
        const float cd  = ComputeCooldown(def, iLevel);
        const float spd = ComputeSpeed (def, iLevel);
        const float sz  = ComputeSize  (def, iLevel);

        std::wstring s = ToW(def.strName) + L"  Lv." + std::to_wstring(iLevel) + L"\n";
        s += L"DMG " + std::to_wstring(dmg);
        if (cnt > 1) s += L"  x" + std::to_wstring(cnt);
        s += L"\n";
        s += (def.eFireMode == FireMode::Sustained)
             ? std::wstring(L"Sustained")
             : (L"Cooldown " + F1(cd) + L"s");
        if (def.fDamageInterval > 0.f) s += L"  tick " + F1(def.fDamageInterval) + L"s";
        s += L"\n";
        s += L"Spd " + F1(spd) + L"  Size " + F1(sz) + L"  Life " + F1(def.fLifetime) + L"s";
        return s;
    }

    // Attack-tower stats (towers.csv) + the weapon it fires WITH that weapon's
    // own level. iTowerLevel = tower level (HP/fire-rate/attack bonuses).
    inline std::wstring TowerStats(const TowerDef* d, int iTowerLevel,
                                   int iWeaponId, int iWeaponLevel)
    {
        std::wstring s = (d ? ToW(d->strName) : std::wstring(L"Tower"));
        s += L"  Lv." + std::to_wstring(iTowerLevel < 1 ? 1 : iTowerLevel) + L"\n";
        if (d)
        {
            s += L"HP " + std::to_wstring(d->iHP);
            if (d->fDefense > 0.f)
                s += L"  Def " + std::to_wstring(static_cast<int>(d->fDefense * 100.f + 0.5f)) + L"%";
            s += L"\nAtk x" + F1(d->fAttack) + L"  Spd x" + F1(d->fAttackSpeed);
            if (d->fCritChance > 0.f)
                s += L"\nCrit " + std::to_wstring(static_cast<int>(d->fCritChance * 100.f + 0.5f)) +
                     L"% x" + F1(d->fCritMult);
            s += L"\nRange " + F1(d->fRange);
        }
        const WeaponDef* pW = (iWeaponId >= 0) ? WeaponDatabase::GetInst().Get(iWeaponId) : nullptr;
        s += L"\nWeapon: " + (pW ? ToW(pW->strName) : std::wstring(L"(none)"));
        if (pW) s += L"  Lv." + std::to_wstring(iWeaponLevel < 1 ? 1 : iWeaponLevel);
        return s;
    }

    // Heal-tower stats; pulse amount scales with the heal-tower level.
    inline std::wstring HealStats(const TowerDef* d, int iLevel)
    {
        if (iLevel < 1) iLevel = 1;
        const int   hp   = d ? d->iHP           : 0;
        const int   amt  = (d ? d->iHealAmount  : 0) * iLevel;
        const float intv = d ? d->fHealInterval : 0.f;
        const float rad  = d ? d->fHealRadius   : 0.f;
        std::wstring s = L"Heal Tower  Lv." + std::to_wstring(iLevel) + L"\n";
        s += L"HP " + std::to_wstring(hp) + L"\n";
        s += L"+" + std::to_wstring(amt) + L" HP / " + F1(intv) + L"s\n";
        s += L"Radius " + F1(rad);
        return s;
    }
}}

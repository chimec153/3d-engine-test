#pragma once

#include "TowerManager.h"
#include "Tower.h"
#include "HealTower.h"
#include "../GameDefs.h"
#include "Scene/Layer.h"
#include "GameObject/GameObject.h"
#include <vector>
#include <algorithm>
#include <memory>

namespace Client
{
    // One numbered tower slot, shared by the tower HUD (display + click) and the
    // placement controller (number keys 1..kMaxTowers). Slots are sorted by
    // acquisition seq so they read in PURCHASE ORDER — attack and heal towers
    // interleaved by when they were bought — and truncated to kMaxTowers.
    struct TowerSlotView
    {
        int  iSeq        = 0;
        bool bHeal       = false;
        int  iTowerId    = -1;   // towers.csv def id (attack; -1 = default type)
        int  iWeaponId   = -1;   // attack equipped weapon (-1 = none); heal = -1
        int  iLevel      = 1;    // TOWER level (heal towers use this too)
        int  iWeaponLevel= 0;    // the equipped WEAPON's own level (0 = unarmed)
        int  eState      = 0;    // 0 placed, 1 ready, 2 down (destroy-cooldown)
        int  iReserveIdx = -1;   // attack reserve index when deployable; else -1

        // Deployable by a key / click: a ready reserve. Attack needs a weapon
        // (iReserveIdx >= 0); heal is fungible (any ready heal reserve).
        bool Deployable() const
        {
            return eState == 1 && (bHeal || iReserveIdx >= 0);
        }
    };

    // Build the ordered slot list. Scans the layer for placed Tower / HealTower
    // objects (they carry live level / weapon + their seq) and TowerManager for
    // the unplaced attack + heal reserves, then sorts by seq and caps at
    // kMaxTowers. Both the HUD and the placement controller call this so the
    // displayed numbering and the number-key mapping always agree.
    inline std::vector<TowerSlotView> BuildTowerSlots(Engine::Layer* pLayer)
    {
        std::vector<TowerSlotView> out;

        // Placed towers (live scene objects).
        if (pLayer)
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive()) continue;
                if (p->GetTag() == "Tower")
                {
                    auto pT = std::static_pointer_cast<Tower>(p);
                    TowerSlotView s;
                    s.iSeq      = pT->GetSlotSeq();
                    s.bHeal     = false;
                    s.iTowerId    = pT->GetTowerDefId();
                    s.iWeaponId   = pT->GetWeaponId();
                    s.iLevel      = pT->GetLevel();          // tower level
                    s.iWeaponLevel= pT->GetWeaponLevel();    // weapon's own level
                    s.eState      = 0;   // placed
                    out.push_back(s);
                }
                else if (p->GetTag() == "HealTower")
                {
                    auto pH = std::static_pointer_cast<HealTower>(p);
                    TowerSlotView s;
                    s.iSeq   = pH->GetSlotSeq();
                    s.bHeal  = true;
                    s.iLevel = pH->GetLevel();   // heal-tower level
                    s.eState = 0;   // placed
                    out.push_back(s);
                }
            }

        auto& mgr = TowerManager::GetInst();

        // Unplaced attack reserves.
        const int nRes = mgr.ReserveCount();
        for (int i = 0; i < nRes; ++i)
        {
            const int  wraw  = mgr.ReserveWeaponRaw(i);
            const bool bDown = mgr.ReserveDown(i);
            TowerSlotView s;
            s.iSeq        = mgr.ReserveSeq(i);
            s.bHeal       = false;
            s.iTowerId    = mgr.ReserveTowerId(i);
            s.iWeaponId   = wraw;
            s.iLevel      = mgr.ReserveLevel(i);   // tower level
            if (auto w = mgr.ReserveWeapon(i)) s.iWeaponLevel = w->iLevel;
            s.eState      = bDown ? 2 : 1;
            // Deployable only when not on cooldown AND a weapon is equipped.
            s.iReserveIdx = (!bDown && wraw >= 0) ? i : -1;
            out.push_back(s);
        }

        // Unplaced heal reserves (no weapon / level / type).
        const int nHeal = mgr.HealReserveCount();
        for (int i = 0; i < nHeal; ++i)
        {
            const bool bDown = mgr.HealReserveDown(i);
            TowerSlotView s;
            s.iSeq   = mgr.HealReserveSeq(i);
            s.bHeal  = true;
            s.iLevel = mgr.HealReserveLevel(i);   // heal-tower level
            s.eState = bDown ? 2 : 1;
            out.push_back(s);
        }

        std::sort(out.begin(), out.end(),
            [](const TowerSlotView& a, const TowerSlotView& b) { return a.iSeq < b.iSeq; });
        if (static_cast<int>(out.size()) > kMaxTowers)
            out.resize(static_cast<size_t>(kMaxTowers));
        return out;
    }
}

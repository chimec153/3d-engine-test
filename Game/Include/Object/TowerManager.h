#pragma once

#include "Weapon.h"
#include <vector>
#include <utility>

namespace Client
{
    // Global "which crafted weapon do towers fire" selection. Every placed
    // Tower reads CurrentWeaponId() each Update, so changing it (from the
    // intermission UI between rounds) re-arms all existing towers at once.
    // Header-only singleton in the GameStateManager / SpawnConfig style:
    // gameplay-wide state that several unrelated objects share without
    // threading a pointer. Seeded by GameScene::Init on stage load.
    class TowerManager
    {
    public:
        // One bought-but-unplaced attack tower. Persists its weapon + level so a
        // tower that gets destroyed keeps both when it's re-placed next round.
        // bDown = destroyed this round → not placeable until the next round
        // starts (OnNewRound clears it). Returned by ConsumePlaceableSlot so the
        // placement controller can seed the fresh Tower's weapon + level.
        struct ReserveTower
        {
            int       iTowerId = -1;   // towers.csv def id; -1 = default attack type
            int       iLevel   = 1;    // TOWER level (the weapon carries its own)
            bool      bDown    = false;
            int       iSeq     = 0;    // acquisition order (stable; HUD/key slot order)
            WeaponPtr pWeapon;         // the weapon object this unplaced tower carries (null = unarmed)
        };

        // One UNPLACED heal tower. Heal towers carry no weapon / level / type, so
        // they only need an acquisition sequence (HUD/key slot order) + the
        // destroy-cooldown flag. Mirrors ReserveTower's role for the attack queue.
        struct HealReserve
        {
            int  iSeq   = 0;
            bool bDown  = false;
            int  iLevel = 1;   // heal-tower level (raised by shop merge)
        };

        static TowerManager& GetInst()
        {
            static TowerManager inst;
            return inst;
        }

        int  CurrentWeaponId() const         { return m_iWeaponId; }
        void SetCurrentWeaponId(int iId)      { m_iWeaponId = iId; }

        // Global tower stat buffs from the level-up menu. Attack + fire-rate are
        // read by every tower at use-time (so all towers benefit immediately);
        // HP + defence are cumulative bonuses applied to a tower at placement
        // (Tower::Init) and pushed onto already-placed towers when the card is
        // picked (Player::ApplyStatUpgrade). All reset for a new game.
        float TowerAtkMult() const           { return m_fTowerAtkMult; }
        float TowerFireRateMult() const      { return m_fTowerFireRateMult; }
        int   TowerBonusHP() const           { return m_iTowerBonusHP; }
        float TowerBonusDef() const          { return m_fTowerBonusDef; }
        void  AddTowerAtk(float f)           { m_fTowerAtkMult += f; }
        void  AddTowerFireRate(float f)      { m_fTowerFireRateMult += f; }
        void  AddTowerHP(int i)              { m_iTowerBonusHP += i; }
        void  AddTowerDef(float f)
        {
            m_fTowerBonusDef += f;
            if (m_fTowerBonusDef > 0.9f) m_fTowerBonusDef = 0.9f;   // matches Attackable clamp
        }

        // Full reset for a new game (game over -> restart). This singleton lives
        // for the whole process, so without an explicit reset the bought-tower
        // counts and the unplaced-tower weapon queue carry over between runs.
        // Called from GameScene::Init alongside Wallet::Reset().
        void Reset()
        {
            m_iTowersOwned     = 0;
            m_iHealTowersOwned = 0;
            m_vecReserve.clear();
            m_vecHealReserve.clear();
            m_iAcqCounter        = 0;
            m_fTowerAtkMult      = 1.f;
            m_fTowerFireRateMult = 1.f;
            m_iTowerBonusHP      = 0;
            m_fTowerBonusDef     = 0.f;
        }

        // Tower inventory: how many towers the player has bought (placed +
        // unplaced + on destroy-cooldown). Bought in the between-round shop;
        // GameScene seeds the starting count. Ownership is permanent — a
        // destroyed tower is NOT lost, it returns to the reserve (benched until
        // next round) and can be re-placed.
        //
        // Each UNPLACED tower is a m_vecReserve entry carrying its weapon, level
        // and destroy-cooldown flag (front = next to be placed, FIFO). Invariant:
        // m_vecReserve.size() == owned - placed, kept by AddTower (buy: +1),
        // ConsumePlaceableSlot (place: -1), DestroyTower (a placed tower died:
        // +1 down entry, owned unchanged) and RemoveTower (sell/merge: owned-1).
        int  TowersOwned() const             { return m_iTowersOwned; }
        // iTowerId = which towers.csv type was bought (-1 = default attack type,
        // resolved at placement by Tower::SetTowerDefId). The no-arg form keeps
        // the pre-type-select behaviour (a generic attack tower).
        void AddTower(int iTowerId = -1)
        {
            ++m_iTowersOwned;
            ReserveTower r;
            r.iTowerId = iTowerId;
            r.iSeq     = m_iAcqCounter++;   // acquisition order
            m_vecReserve.push_back(r);   // new unplaced tower, weapon unconfigured, level 1
        }
        void SetTowersOwned(int iCount)
        {
            m_iTowersOwned = iCount < 0 ? 0 : iCount;
            // Seeded at stage init (nothing placed yet): one unconfigured
            // reserve slot per owned tower, each with its own acquisition seq.
            m_vecReserve.clear();
            for (int i = 0; i < m_iTowersOwned; ++i)
            {
                ReserveTower r;
                r.iSeq = m_iAcqCounter++;
                m_vecReserve.push_back(r);
            }
        }
        // A placed tower was SOLD or MERGED away: drop it from the owned count
        // (no reserve entry — the tower is gone for good). Used by the shop sell
        // + the tower-merge consume. NOT the death path (see DestroyTower).
        void RemoveTower()                   { if (m_iTowersOwned > 0) --m_iTowersOwned; }

        // A placed tower was DESTROYED by enemies: keep ownership (no --owned)
        // but push its weapon + level back into the reserve as a "down" slot.
        // It can't be re-placed until the next round (OnNewRound clears bDown),
        // and it keeps its merged level + assigned weapon when re-placed.
        void DestroyTower(int iLevel, int iTowerId, int iSeq, WeaponPtr pWeapon)
        {
            ReserveTower r;
            r.iTowerId  = iTowerId;
            r.iLevel    = iLevel < 1 ? 1 : iLevel;   // tower level
            r.bDown     = true;
            // Preserve the tower's acquisition seq so it keeps its HUD/key slot
            // position when it returns to the reserve (re-placeable next round).
            r.iSeq      = (iSeq >= 0) ? iSeq : m_iAcqCounter++;
            r.pWeapon   = std::move(pWeapon);   // the tower's weapon comes back with it
            m_vecReserve.push_back(std::move(r));
        }
        // Round start: destroyed towers (attack + heal) come back online.
        void OnNewRound()
        {
            for (auto& r : m_vecReserve)     r.bDown = false;
            for (auto& h : m_vecHealReserve) h.bDown = false;
        }
        // Acquisition seq of the i-th reserve slot (for the HUD/key slot order).
        int  ReserveSeq(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i].iSeq : 0;
        }
        // Reserve slots that can be placed right now: not on destroy-cooldown AND
        // carrying an equipped weapon. A freshly bought tower starts weaponless
        // (iWeaponId < 0) and is NOT placeable until a weapon is equipped in the
        // shop. The placement controller gates the build cursor on this.
        int  PlaceableTowerCount() const
        {
            int n = 0;
            for (const auto& r : m_vecReserve) if (!r.bDown && r.pWeapon) ++n;
            return n;
        }

        // Per-unplaced-tower weapon config (shop UI). The reserve is the queue
        // of bought-but-unplaced towers; index 0 is the next one placed.
        int  ReserveCount() const            { return static_cast<int>(m_vecReserve.size()); }
        // Def id of the i-th reserve's weapon object, or -1 if unarmed (display).
        int  ReserveWeaponRaw(int i) const
        {
            if (i < 0 || i >= static_cast<int>(m_vecReserve.size())) return -1;
            return m_vecReserve[i].pWeapon ? m_vecReserve[i].pWeapon->iWeaponId : -1;
        }
        // The weapon OBJECT the i-th reserve carries (null = unarmed).
        WeaponPtr ReserveWeapon(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i].pWeapon : nullptr;
        }
        int  ReserveLevel(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i].iLevel : 1;
        }
        // towers.csv type id of the i-th reserve slot (-1 = default attack type).
        int  ReserveTowerId(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i].iTowerId : -1;
        }
        bool ReserveDown(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) && m_vecReserve[i].bDown;
        }
        // Install / replace the weapon object on the i-th reserve (the shop moves
        // a player weapon here). Returns the previously held weapon (or null) so
        // the caller can hand it back to the player.
        WeaponPtr SetReserveWeapon(int i, const WeaponPtr& pWeapon)
        {
            if (i < 0 || i >= static_cast<int>(m_vecReserve.size())) return nullptr;
            WeaponPtr old = m_vecReserve[i].pWeapon;
            m_vecReserve[i].pWeapon = pWeapon;
            return old;
        }
        // Bump an unplaced attack tower's level (shop merge into a reserve copy).
        // Clamped to >= 1; the caller enforces the upper cap.
        void AddReserveLevel(int i, int iDelta)
        {
            if (i < 0 || i >= static_cast<int>(m_vecReserve.size())) return;
            int lv = m_vecReserve[i].iLevel + iDelta;
            m_vecReserve[i].iLevel = lv < 1 ? 1 : lv;
        }
        // Remove an unplaced attack tower (shop sell / merge-consume of a reserve
        // copy): drop the entry and the owned count. Mirrors RemoveTower, which
        // only decrements (a placed tower has no reserve entry).
        void RemoveReserveTower(int i)
        {
            if (i < 0 || i >= static_cast<int>(m_vecReserve.size())) return;
            m_vecReserve.erase(m_vecReserve.begin() + i);
            if (m_iTowersOwned > 0) --m_iTowersOwned;
        }
        // Remove an unplaced heal tower (shop sell of a reserve heal copy).
        void RemoveHealReserveAt(int i)
        {
            if (i < 0 || i >= static_cast<int>(m_vecHealReserve.size())) return;
            m_vecHealReserve.erase(m_vecHealReserve.begin() + i);
            if (m_iHealTowersOwned > 0) --m_iHealTowersOwned;
        }
        // Placement pops the front-most PLACEABLE reserve slot: not on cooldown
        // and with an equipped weapon (iWeaponId >= 0). A weapon is NO LONGER
        // auto-assigned — a weaponless tower can't be placed (PlaceableTowerCount
        // gates the build cursor, so the no-match fallback below shouldn't fire).
        ReserveTower ConsumePlaceableSlot()
        {
            for (size_t i = 0; i < m_vecReserve.size(); ++i)
            {
                if (m_vecReserve[i].bDown || !m_vecReserve[i].pWeapon) continue;
                ReserveTower r = std::move(m_vecReserve[i]);
                m_vecReserve.erase(m_vecReserve.begin() + i);
                return r;
            }
            // No placeable (weaponed) slot — return a weaponless sentinel. The
            // placement controller gates on PlaceableTowerCount, so this path is
            // defensive only; the caller must not place a weaponless tower.
            return ReserveTower{};
        }
        // Place a SPECIFIC reserve slot by index — the tower HUD lets the player
        // pick WHICH unplaced tower to deploy (their type / weapon / level can
        // differ). Erases and returns that slot when it's placeable (not on
        // cooldown, weapon equipped); otherwise falls back to the front-most
        // placeable slot (ConsumePlaceableSlot). iIndex < 0 also falls back.
        ReserveTower ConsumePlaceableSlotAt(int iIndex)
        {
            if (iIndex >= 0 && iIndex < static_cast<int>(m_vecReserve.size()) &&
                !m_vecReserve[iIndex].bDown && m_vecReserve[iIndex].pWeapon)
            {
                ReserveTower r = std::move(m_vecReserve[iIndex]);
                m_vecReserve.erase(m_vecReserve.begin() + iIndex);
                return r;
            }
            return ConsumePlaceableSlot();
        }
        // The towers.csv type id that ConsumePlaceableSlotAt(iIndex) WOULD deploy,
        // WITHOUT consuming it — lets the placement ghost match the chosen tower's
        // shape. Falls back to the front-most placeable slot (iIndex < 0 or that
        // slot isn't placeable); -1 (default attack type) when nothing's placeable.
        int PeekPlaceableTowerId(int iIndex) const
        {
            if (iIndex >= 0 && iIndex < static_cast<int>(m_vecReserve.size()) &&
                !m_vecReserve[iIndex].bDown && m_vecReserve[iIndex].pWeapon)
                return m_vecReserve[iIndex].iTowerId;
            for (const auto& r : m_vecReserve)
                if (!r.bDown && r.pWeapon) return r.iTowerId;
            return -1;
        }

        // Heal-tower budget — separate from attack towers (bought separately in
        // the shop). Each UNPLACED heal tower is an m_vecHealReserve entry (with
        // its acquisition seq), mirroring the attack reserve so heal towers take
        // their own numbered HUD/key slot. Invariant: heal reserve size ==
        // owned - placed.
        int  HealTowersOwned() const         { return m_iHealTowersOwned; }
        void AddHealTower()
        {
            ++m_iHealTowersOwned;
            HealReserve h;
            h.iSeq = m_iAcqCounter++;
            m_vecHealReserve.push_back(h);
        }
        void SetHealTowersOwned(int iCount)
        {
            m_iHealTowersOwned = iCount < 0 ? 0 : iCount;
            m_vecHealReserve.clear();
            for (int i = 0; i < m_iHealTowersOwned; ++i)
            {
                HealReserve h;
                h.iSeq = m_iAcqCounter++;
                m_vecHealReserve.push_back(h);
            }
        }
        // Shop SELL of a placed heal tower: give up ownership for good (the sold
        // tower was placed, so it isn't in the reserve — just drop the count).
        void RemoveHealTower()               { if (m_iHealTowersOwned > 0) --m_iHealTowersOwned; }
        // Heal tower DESTROYED by enemies: keep ownership but bench it until the
        // next round (mirrors DestroyTower). Preserves its acquisition seq so it
        // keeps its slot position when re-placeable next round.
        void DestroyHealTower(int iSeq, int iLevel)
        {
            HealReserve h;
            h.bDown  = true;
            h.iSeq   = (iSeq >= 0) ? iSeq : m_iAcqCounter++;
            h.iLevel = iLevel < 1 ? 1 : iLevel;   // keeps its merged level
            m_vecHealReserve.push_back(h);
        }
        int  HealTowersDown() const
        {
            int n = 0;
            for (const auto& h : m_vecHealReserve) if (h.bDown) ++n;
            return n;
        }
        // Heal towers placeable right now = unplaced heal reserves not on cooldown.
        int  PlaceableHealCount() const
        {
            int n = 0;
            for (const auto& h : m_vecHealReserve) if (!h.bDown) ++n;
            return n;
        }
        // Pop the front-most placeable (not-down) heal reserve, returning it (seq
        // + level) so the placed HealTower keeps its slot + level. iSeq < 0 in the
        // returned struct means none placeable (caller gates on PlaceableHealCount).
        HealReserve ConsumePlaceableHeal()
        {
            for (size_t i = 0; i < m_vecHealReserve.size(); ++i)
            {
                if (m_vecHealReserve[i].bDown) continue;
                HealReserve h = m_vecHealReserve[i];
                m_vecHealReserve.erase(m_vecHealReserve.begin() + i);
                return h;
            }
            HealReserve none; none.iSeq = -1; return none;
        }
        // Heal reserve enumeration for the HUD/key slot builder.
        int  HealReserveCount() const        { return static_cast<int>(m_vecHealReserve.size()); }
        int  HealReserveSeq(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecHealReserve.size())) ? m_vecHealReserve[i].iSeq : 0;
        }
        bool HealReserveDown(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecHealReserve.size())) && m_vecHealReserve[i].bDown;
        }
        int  HealReserveLevel(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecHealReserve.size())) ? m_vecHealReserve[i].iLevel : 1;
        }
        // Bump an unplaced heal tower's level (shop merge into a reserve heal copy).
        void AddHealReserveLevel(int i, int iDelta)
        {
            if (i < 0 || i >= static_cast<int>(m_vecHealReserve.size())) return;
            int lv = m_vecHealReserve[i].iLevel + iDelta;
            m_vecHealReserve[i].iLevel = lv < 1 ? 1 : lv;
        }

    private:
        TowerManager() = default;
        ~TowerManager() = default;
        TowerManager(const TowerManager&)            = delete;
        TowerManager& operator=(const TowerManager&) = delete;

        // -1 = no weapon chosen yet (towers don't fire). GameScene seeds a
        // sensible default at scene init.
        int m_iWeaponId        = -1;
        int m_iTowersOwned     = 0;
        int m_iHealTowersOwned = 0;
        // Monotonic acquisition counter — every bought tower (attack or heal)
        // gets the next value, defining the numbered HUD/key slot order.
        int m_iAcqCounter      = 0;

        // Level-up tower buffs (see accessors above).
        float m_fTowerAtkMult      = 1.f;
        float m_fTowerFireRateMult = 1.f;
        int   m_iTowerBonusHP      = 0;
        float m_fTowerBonusDef     = 0.f;

        // One entry per unplaced attack tower (front = next placed). Carries the
        // tower's weapon + level + destroy-cooldown flag. Size tracks
        // owned - placed (see AddTower / ConsumePlaceableSlot / DestroyTower).
        std::vector<ReserveTower> m_vecReserve;
        // One entry per unplaced heal tower (see the heal-tower section above).
        std::vector<HealReserve>  m_vecHealReserve;
    };
}

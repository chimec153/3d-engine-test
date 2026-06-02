#pragma once

#include <vector>

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
            int  iTowerId  = -1;   // towers.csv def id; -1 = default attack type
            int  iWeaponId = -1;   // -1 = inherit CurrentWeaponId
            int  iLevel    = 1;
            bool bDown     = false;
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
            m_iHealTowersDown  = 0;
            m_vecReserve.clear();
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
            m_vecReserve.push_back(r);   // new unplaced tower, weapon unconfigured, level 1
        }
        void SetTowersOwned(int iCount)
        {
            m_iTowersOwned = iCount < 0 ? 0 : iCount;
            // Seeded at stage init (nothing placed yet): one unconfigured
            // reserve slot per owned tower.
            m_vecReserve.assign(static_cast<size_t>(m_iTowersOwned), ReserveTower{});
        }
        // A placed tower was SOLD or MERGED away: drop it from the owned count
        // (no reserve entry — the tower is gone for good). Used by the shop sell
        // + the tower-merge consume. NOT the death path (see DestroyTower).
        void RemoveTower()                   { if (m_iTowersOwned > 0) --m_iTowersOwned; }

        // A placed tower was DESTROYED by enemies: keep ownership (no --owned)
        // but push its weapon + level back into the reserve as a "down" slot.
        // It can't be re-placed until the next round (OnNewRound clears bDown),
        // and it keeps its merged level + assigned weapon when re-placed.
        void DestroyTower(int iWeaponId, int iLevel, int iTowerId = -1)
        {
            ReserveTower r;
            r.iTowerId  = iTowerId;
            r.iWeaponId = iWeaponId;
            r.iLevel    = iLevel < 1 ? 1 : iLevel;
            r.bDown     = true;
            m_vecReserve.push_back(r);
        }
        // Round start: destroyed towers come back online (placeable again).
        void OnNewRound()
        {
            for (auto& r : m_vecReserve) r.bDown = false;
            m_iHealTowersDown = 0;
        }
        // Reserve slots that can be placed right now: not on destroy-cooldown AND
        // carrying an equipped weapon. A freshly bought tower starts weaponless
        // (iWeaponId < 0) and is NOT placeable until a weapon is equipped in the
        // shop. The placement controller gates the build cursor on this.
        int  PlaceableTowerCount() const
        {
            int n = 0;
            for (const auto& r : m_vecReserve) if (!r.bDown && r.iWeaponId >= 0) ++n;
            return n;
        }

        // Per-unplaced-tower weapon config (shop UI). The reserve is the queue
        // of bought-but-unplaced towers; index 0 is the next one placed.
        int  ReserveCount() const            { return static_cast<int>(m_vecReserve.size()); }
        // Raw stored id (-1 = inherit CurrentWeaponId); caller resolves for display.
        int  ReserveWeaponRaw(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i].iWeaponId : -1;
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
        void SetReserveWeapon(int i, int iId)
        {
            if (i >= 0 && i < static_cast<int>(m_vecReserve.size())) m_vecReserve[i].iWeaponId = iId;
        }
        // Placement pops the front-most PLACEABLE reserve slot: not on cooldown
        // and with an equipped weapon (iWeaponId >= 0). A weapon is NO LONGER
        // auto-assigned — a weaponless tower can't be placed (PlaceableTowerCount
        // gates the build cursor, so the no-match fallback below shouldn't fire).
        ReserveTower ConsumePlaceableSlot()
        {
            for (size_t i = 0; i < m_vecReserve.size(); ++i)
            {
                if (m_vecReserve[i].bDown || m_vecReserve[i].iWeaponId < 0) continue;
                ReserveTower r = m_vecReserve[i];
                m_vecReserve.erase(m_vecReserve.begin() + i);
                return r;
            }
            // No placeable (weaponed) slot — return a weaponless sentinel. The
            // placement controller gates on PlaceableTowerCount, so this path is
            // defensive only; the caller must not place a weaponless tower.
            return ReserveTower{};
        }

        // Heal-tower budget — separate from attack towers (bought separately
        // in the shop, placed with a different key).
        int  HealTowersOwned() const         { return m_iHealTowersOwned; }
        void AddHealTower()                  { ++m_iHealTowersOwned; }
        void SetHealTowersOwned(int iCount)  { m_iHealTowersOwned = iCount < 0 ? 0 : iCount; m_iHealTowersDown = 0; }
        // Shop SELL of a placed heal tower: give up ownership for good.
        void RemoveHealTower()               { if (m_iHealTowersOwned > 0) --m_iHealTowersOwned; }
        // Heal tower DESTROYED by enemies: keep ownership but bench it until the
        // next round (mirrors DestroyTower; heal towers have no weapon/level so
        // a plain count is enough). PlaceableHealCount excludes these.
        void DestroyHealTower()              { ++m_iHealTowersDown; }
        int  HealTowersDown() const          { return m_iHealTowersDown; }
        // Heal towers placeable right now = owned, minus those on cooldown.
        int  PlaceableHealCount() const
        {
            const int n = m_iHealTowersOwned - m_iHealTowersDown;
            return n < 0 ? 0 : n;
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
        int m_iHealTowersDown  = 0;   // heal towers destroyed this round (cooldown)

        // Level-up tower buffs (see accessors above).
        float m_fTowerAtkMult      = 1.f;
        float m_fTowerFireRateMult = 1.f;
        int   m_iTowerBonusHP      = 0;
        float m_fTowerBonusDef     = 0.f;

        // One entry per unplaced attack tower (front = next placed). Carries the
        // tower's weapon + level + destroy-cooldown flag. Size tracks
        // owned - placed (see AddTower / ConsumePlaceableSlot / DestroyTower).
        std::vector<ReserveTower> m_vecReserve;
    };
}

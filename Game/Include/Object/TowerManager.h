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
            m_fTowerAtkMult      = 1.f;
            m_fTowerFireRateMult = 1.f;
            m_iTowerBonusHP      = 0;
            m_fTowerBonusDef     = 0.f;
        }

        // Tower inventory: how many towers the player has bought (placed +
        // unplaced). The placement controller only lets you place up to this
        // many (placed count is the live "Tower" objects in the scene). Bought
        // in the between-round shop; GameScene seeds the starting count.
        //
        // Each UNPLACED tower also carries a chosen weapon id in m_vecReserve
        // (front = next to be placed, FIFO). -1 = "inherit the current default"
        // (CurrentWeaponId) — the pre-feature behaviour. Invariant:
        // m_vecReserve.size() == owned - placed, kept by AddTower (buy: +1),
        // ConsumeReserveWeapon (place: -1) and RemoveTower (a *placed* tower
        // died: owned-1, reserve untouched).
        int  TowersOwned() const             { return m_iTowersOwned; }
        void AddTower()
        {
            ++m_iTowersOwned;
            m_vecReserve.push_back(-1);   // new unplaced tower, weapon unconfigured
        }
        void SetTowersOwned(int iCount)
        {
            m_iTowersOwned = iCount < 0 ? 0 : iCount;
            // Seeded at stage init (nothing placed yet): one unconfigured
            // reserve slot per owned tower.
            m_vecReserve.assign(static_cast<size_t>(m_iTowersOwned), -1);
        }
        // A placed tower was destroyed: drop it from the owned count so the
        // freed cell can't be re-used for free — the player must re-buy. The
        // reserve is untouched (the dead tower was placed, not in reserve).
        void RemoveTower()                   { if (m_iTowersOwned > 0) --m_iTowersOwned; }

        // Per-unplaced-tower weapon config (shop UI). The reserve is the queue
        // of bought-but-unplaced towers; index 0 is the next one placed.
        int  ReserveCount() const            { return static_cast<int>(m_vecReserve.size()); }
        // Raw stored id (-1 = inherit CurrentWeaponId); caller resolves for display.
        int  ReserveWeaponRaw(int i) const
        {
            return (i >= 0 && i < static_cast<int>(m_vecReserve.size())) ? m_vecReserve[i] : -1;
        }
        void SetReserveWeapon(int i, int iId)
        {
            if (i >= 0 && i < static_cast<int>(m_vecReserve.size())) m_vecReserve[i] = iId;
        }
        // Placement pops the front reserve slot and returns the resolved weapon
        // id (its stored id, or the current default when unconfigured / empty).
        int  ConsumeReserveWeapon()
        {
            if (m_vecReserve.empty()) return m_iWeaponId;
            const int iId = m_vecReserve.front();
            m_vecReserve.erase(m_vecReserve.begin());
            return iId < 0 ? m_iWeaponId : iId;
        }

        // Heal-tower budget — separate from attack towers (bought separately
        // in the shop, placed with a different key).
        int  HealTowersOwned() const         { return m_iHealTowersOwned; }
        void AddHealTower()                  { ++m_iHealTowersOwned; }
        void SetHealTowersOwned(int iCount)  { m_iHealTowersOwned = iCount < 0 ? 0 : iCount; }
        void RemoveHealTower()               { if (m_iHealTowersOwned > 0) --m_iHealTowersOwned; }

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

        // Level-up tower buffs (see accessors above).
        float m_fTowerAtkMult      = 1.f;
        float m_fTowerFireRateMult = 1.f;
        int   m_iTowerBonusHP      = 0;
        float m_fTowerBonusDef     = 0.f;

        // Weapon id per unplaced attack tower (front = next placed). -1 = use
        // the current default. Size tracks owned - placed (see AddTower etc.).
        std::vector<int> m_vecReserve;
    };
}

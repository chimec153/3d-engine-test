#pragma once

namespace Client
{
    // Player money earned by collecting orbs (enemy drops) and spent in the
    // between-round shop (TowerIntermissionUI). Header-only singleton in the
    // GameStateManager / TowerManager style: a single cross-cutting counter
    // the Player (earn), HUD (display), and shop (spend) all touch without
    // threading a pointer. Reset to 0 by GameScene::Init each stage.
    class Wallet
    {
    public:
        static Wallet& GetInst()
        {
            static Wallet inst;
            return inst;
        }

        int  Money() const          { return m_iMoney; }
        void Add(int iAmount)        { if (iAmount > 0) m_iMoney += iAmount; }
        // Deduct iCost if affordable; returns false (and leaves the balance
        // untouched) when there isn't enough money.
        bool TrySpend(int iCost)
        {
            if (iCost < 0 || m_iMoney < iCost) return false;
            m_iMoney -= iCost;
            return true;
        }
        void Reset()                { m_iMoney = 0; }

    private:
        Wallet() = default;
        ~Wallet() = default;
        Wallet(const Wallet&)            = delete;
        Wallet& operator=(const Wallet&) = delete;

        int m_iMoney = 0;
    };
}

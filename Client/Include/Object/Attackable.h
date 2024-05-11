#pragma once
#include "Bindable\Drawable.h"

namespace Engine
{
    class PaperBurn;
    class Particle;
    class SoundBindable;
}

namespace Client
{
    class Attackable :
        public Engine::Drawable
    {
    public:
        Attackable(int iMaxHP, int iAttackMin, int iAttackMax);
        virtual ~Attackable() override = default;

    private:
        int m_iMaxHP;
        int m_iHP;
        int m_iAttackMin;
        int m_iAttackMax;
        std::shared_ptr<Engine::PaperBurn> m_pPaperBurn;
        std::shared_ptr<Engine::Particle>   m_pParticle;
        std::shared_ptr<Engine::Particle>   m_pBloodParticle;
        std::shared_ptr<Engine::SoundBindable> m_pHitSound;

    public:
        bool Attack(Attackable* pHit)    const;
        int GetAttack() const;
        void StartPaperBurn();
        std::shared_ptr<Engine::PaperBurn> GetPaperBurn()   const;
        std::shared_ptr<Engine::Particle> GetParticle() const;

    public:
        virtual bool Init() override;


    };

}
#pragma once
#include "Component\Component.h"

namespace Engine
{
    class PaperBurn;
    class Particle;
    class SoundBindable;
    class Drawable;
}

namespace Client
{
    // Phase E5 — Attackable migrated from a Drawable subclass to a
    // behavioral Component. Player/Monster (still Drawable subclasses)
    // attach an Attackable in their Init via CreateComponent. The
    // PaperBurn / Particle / SoundBindable that used to be children of
    // the Attackable-as-Drawable are now siblings on the owning Drawable
    // — Attackable creates them by calling GetOwner()->CreateComponent
    // during its Init.
    //
    // Migration to a full GameObject-based Player/Monster (which would
    // also require Animation::AddSocket / JointSocket to accept GameObjects)
    // is deferred to a separate phase.
    class Attackable :
        public Engine::Component
    {
    public:
        Attackable();
        Attackable(int iMaxHP, int iAttackMin, int iAttackMax);
        Attackable(const Attackable& other);
        virtual ~Attackable() override = default;

    private:
        int m_iMaxHP;
        int m_iHP;
        int m_iAttackMin;
        int m_iAttackMax;
        std::shared_ptr<Engine::PaperBurn>     m_pPaperBurn;
        std::shared_ptr<Engine::Particle>      m_pParticle;
        std::shared_ptr<Engine::Particle>      m_pBloodParticle;
        std::shared_ptr<Engine::SoundBindable> m_pHitSound;

    public:
        // Attack a target's Attackable component directly. Returns true
        // if the target died from this hit. Caller resolves the target's
        // Attackable via FindComponent (Drawable side) or GetComponent
        // (GameObject side) so this works regardless of host type.
        bool Attack(Attackable* pTargetAttackable) const;
        int GetAttack() const;
        void StartPaperBurn();

        std::shared_ptr<Engine::PaperBurn> GetPaperBurn()   const { return m_pPaperBurn; }
        std::shared_ptr<Engine::Particle>  GetParticle()    const { return m_pParticle; }

    public:
        virtual bool Init() override;
        virtual std::shared_ptr<Engine::Component> Clone() override;
    };

}

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
        // bWithBloodParticle — opt-in for the on-hit blood particle
        // emitter. Default false because every Attackable that opts in
        // costs a per-frame CS dispatch + GPU upload (visible in the
        // profile at ~6% when 30+ enemies each ran one). Only entities
        // that actually need the visual (e.g. the player) should pass
        // true.
        Attackable(int iMaxHP, int iAttackMin, int iAttackMax, bool bWithBloodParticle = false);
        Attackable(const Attackable& other);
        virtual ~Attackable() override = default;

    private:
        int m_iMaxHP;
        int m_iHP;
        int m_iAttackMin;
        int m_iAttackMax;
        bool m_bWithBloodParticle = false;
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
        int GetHP()    const { return m_iHP; }
        int GetMaxHP() const { return m_iMaxHP; }
        void StartPaperBurn();

        // Stat-boost API — Player::ConsumeLevelUp routes the player's
        // chosen card here. AddMaxHP also tops the current HP so a
        // boost mid-combat acts like a heal.
        void AddMaxHP(int iDelta)
        {
            m_iMaxHP += iDelta;
            m_iHP    += iDelta;
            if (m_iHP > m_iMaxHP) m_iHP = m_iMaxHP;
            if (m_iHP < 0)        m_iHP = 0;
        }
        void AddAttack(int iDelta)
        {
            m_iAttackMin += iDelta;
            m_iAttackMax += iDelta;
            if (m_iAttackMin < 0) m_iAttackMin = 0;
            if (m_iAttackMax < m_iAttackMin) m_iAttackMax = m_iAttackMin;
        }

        std::shared_ptr<Engine::PaperBurn> GetPaperBurn()   const { return m_pPaperBurn; }
        std::shared_ptr<Engine::Particle>  GetParticle()    const { return m_pParticle; }

    public:
        virtual bool Init() override;
        // Per-frame sync of the sibling Particle emitters' standalone
        // Transforms to the host GameObject's Transform. Without this
        // the particles spawn at world origin (Particle::Init creates a
        // fresh Transform at (0,0,0) and never updates it from anywhere).
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Engine::Component> Clone() override;
    };

}

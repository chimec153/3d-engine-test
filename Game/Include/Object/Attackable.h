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
        // bWithPaperBurn — opt-in for the sibling PaperBurn dissolve component.
        // Default true preserves Player/legacy. Enemy passes false: enemies do
        // their dissolve per-instance via EnemyMeshRenderer's instance stream,
        // and a PaperBurn sibling would force RenderManager's instancing fast
        // path to bail (it conservatively skips buckets with decorator
        // siblings), dropping every enemy back to a solo draw.
        // bWithImpactBurst — opt-in for the animated impact-flash flipbook
        // (burst.png). Separate from bWithBloodParticle so mechanical hosts
        // (towers) can get the hit flash without spawning blood/dust.
        Attackable(int iMaxHP, int iAttackMin, int iAttackMax, bool bWithBloodParticle = false, bool bWithPaperBurn = true, bool bWithImpactBurst = false);
        Attackable(const Attackable& other);
        virtual ~Attackable() override = default;

    private:
        int m_iMaxHP;
        int m_iHP;
        int m_iAttackMin;
        int m_iAttackMax;
        // Fractional reduction applied to incoming damage in Attack (0 = none).
        float m_fDamageReduction = 0.f;
        bool m_bWithBloodParticle = false;
        bool m_bWithPaperBurn = true;
        bool m_bWithImpactBurst = false;
        std::shared_ptr<Engine::PaperBurn>     m_pPaperBurn;
        std::shared_ptr<Engine::Particle>      m_pParticle;
        std::shared_ptr<Engine::Particle>      m_pBloodParticle;
        // Companion to the blood emitter — a grey impact puff. Created and
        // emitted under the same bWithBloodParticle opt-in so only the
        // player pays for it (same per-frame CS dispatch cost rationale).
        std::shared_ptr<Engine::Particle>      m_pDustParticle;
        // Animated impact flash — a square N×N flipbook (white/grayscale +
        // alpha) that plays once per hit. The flipbook frame advances with
        // particle age in the GS, so the animation IS the visual expansion;
        // size/velocity are kept ~constant. Same opt-in / cost rationale.
        std::shared_ptr<Engine::Particle>      m_pImpactParticle;
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
        // Restore current HP (heal tower). Clamped to max; never revives a
        // dead (HP<=0) target. Returns the HP actually restored (0 if already
        // full / dead) so the caller only fires heal feedback when it did
        // something.
        int Heal(int iAmount)
        {
            if (iAmount <= 0 || m_iHP <= 0) return 0;
            const int iBefore = m_iHP;
            m_iHP += iAmount;
            if (m_iHP > m_iMaxHP) m_iHP = m_iMaxHP;
            return m_iHP - iBefore;
        }
        void AddAttack(int iDelta)
        {
            m_iAttackMin += iDelta;
            m_iAttackMax += iDelta;
            if (m_iAttackMin < 0) m_iAttackMin = 0;
            if (m_iAttackMax < m_iAttackMin) m_iAttackMax = m_iAttackMin;
        }
        // Fractional incoming-damage reduction (0..0.9). Attack() applies the
        // TARGET's reduction, so the player's "defense up" stat softens hits.
        void AddDamageReduction(float fDelta)
        {
            m_fDamageReduction += fDelta;
            if (m_fDamageReduction < 0.f)  m_fDamageReduction = 0.f;
            if (m_fDamageReduction > 0.9f) m_fDamageReduction = 0.9f;
        }
        float GetDamageReduction() const { return m_fDamageReduction; }
        // Absolute setter — used by Enemy::ApplyDef to push a CSV-loaded
        // damage range over the default the ctor baked in. Clamps to a
        // sane order so a malformed (min > max) row doesn't break Attack.
        void SetAttackRange(int iMin, int iMax)
        {
            m_iAttackMin = iMin < 0 ? 0 : iMin;
            m_iAttackMax = iMax < m_iAttackMin ? m_iAttackMin : iMax;
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

#pragma once
#include "GameObject/GameObject.h"
#include "WeaponData.h"
#include "WeaponDatabase.h"
#include "Enemy.h"
#include "Vector3.h"
#include "Types.h"
#include "Bindable/Transform.h"
#include "Vfx/BeamRenderManager.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "Core/Macro.h"
#include <memory>
#include <cmath>

namespace Client
{
    // A laser beam: a persistent damaging line from the player toward the aim
    // direction. Created for Straight + Sustained weapons -- the Sustained
    // model spawns one persistent instance, and a flying Straight bullet would
    // just leave the arena, so a beam stays anchored to the player instead.
    // Header-only (created only via CreateGameObject<Beam>, no factory needed).
    // No collider: the Player drives it each frame (it owns the cursor aim) and
    // the beam ticks damage to enemies along its segment itself.
    class Beam : public Engine::GameObject
    {
    public:
        Beam() = default;
        virtual ~Beam() override = default;

        std::shared_ptr<Engine::Transform> GetTransform() const { return m_pTransform; }

        // Aim-lock: when on, the beam latches its heading at the start of each
        // "on" pulse and ignores the per-frame yaw passed to Drive until the
        // pulse ends -- so once the beam fires it points straight and does not
        // turn. Off (default) the beam tracks the live yaw every frame (the
        // player's cursor-swept laser). Towers set this so their auto-aimed
        // beam fires a fixed straight shot, re-acquiring only between pulses.
        void SetAimLock(bool bLock) { m_bAimLock = bLock; }

        void Configure(int iWeaponId, int iLevel)
        {
            m_iWeaponId = iWeaponId;
            m_iLevel    = iLevel;
            if (const WeaponDef* d = WeaponDatabase::GetInst().Get(iWeaponId))
            {
                const float r = ((d->uColorRGB >> 16) & 0xFF) / 255.f;
                const float g = ((d->uColorRGB >> 8)  & 0xFF) / 255.f;
                const float b = ( d->uColorRGB        & 0xFF) / 255.f;
                m_vColor = Engine::Vector3(r, g, b);
            }
        }

        virtual bool Init() override
        {
            if (!__super::Init()) return false;
            // No renderer: the visual is drawn by BeamRenderManager as a
            // camera-facing additive billboard (see Drive's Submit). A bare
            // Transform is kept so the GameObject has its usual world node.
            m_pTransform = AddComponent<Engine::Transform>("transform");
            return true;
        }

        // Player calls this every frame with the beam origin (muzzle) + aim yaw.
        // Positions/orients/scales the visual and ticks segment damage.
        void Drive(const Engine::Vector3& vAnchor, float fAimYaw, float fDeltaTime)
        {
            const WeaponDef* pDef = WeaponDatabase::GetInst().Get(m_iWeaponId);
            if (!pDef || !m_pTransform) return;

            // Duty cycle: pulse on for fLifetime seconds, then off for
            // fCooldown seconds. Either being <= 0 disables the cycle and the
            // beam stays on continuously (legacy always-on behaviour). The
            // off phase halves average DPS and reads as a visible blink.
            const float fOn  = pDef->fLifetime;
            const float fOff = pDef->fCooldown;
            if (fOn > 0.f && fOff > 0.f)
            {
                m_fPhaseAcc += fDeltaTime;
                const float fPhaseLen = m_bOn ? fOn : fOff;
                if (m_fPhaseAcc >= fPhaseLen)
                {
                    m_fPhaseAcc -= fPhaseLen;
                    m_bOn = !m_bOn;
                    m_fTickAcc = 0.f;   // fresh tick window each on-phase
                }
            }
            else
                m_bOn = true;

            // Aim-lock: latch the heading at the first frame of each on-pulse
            // and hold it until the pulse ends; off-mode tracks the live yaw.
            float fYaw = fAimYaw;
            if (m_bAimLock)
            {
                if (m_bOn)
                {
                    if (!m_bYawLatched) { m_fLockedYaw = fAimYaw; m_bYawLatched = true; }
                    fYaw = m_fLockedYaw;
                }
                else
                    m_bYawLatched = false;   // re-acquire on the next on-pulse
            }

            const float fWidth = (pDef->fSize > 0.f ? pDef->fSize : 0.25f) * 2.f;
            const Engine::Vector3 vDir(-sinf(fYaw), 0.f, -cosf(fYaw));

            if (!m_bOn) return;   // off phase: no visual, no damage

            // Queue the visual for this frame — a camera-facing additive
            // billboard from the anchor forward kBeamLen. Half-width = fSize.
            // Done every frame (not gated by the damage tick below).
            const Engine::Vector3 vEnd = vAnchor + vDir * kBeamLen;
            BeamRenderManager::GetInst()->Submit(vAnchor, vEnd, fWidth * 0.5f, m_vColor);

            // Tick damage along the segment [anchor, anchor + dir*kBeamLen].
            const float fInterval = pDef->fDamageInterval > 0.f ? pDef->fDamageInterval : 0.1f;
            m_fTickAcc += fDeltaTime;
            if (m_fTickAcc < fInterval) return;
            m_fTickAcc -= fInterval;

            const int   iDmg   = ComputeDamage(*pDef, m_iLevel);
            const float fHalfW = fWidth * 0.5f + 0.3f;   // a little forgiving
            auto pScene = Engine::SceneManager::GetInst()->GetScene();
            if (!pScene) return;
            auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
            if (!pLayer) return;
            for (const auto& p : pLayer->GetGameObjectList())
            {
                if (!p || !p->IsActive() || p->GetTag() != "Enemy") continue;
                auto* pEnemy = dynamic_cast<Enemy*>(p.get());
                if (!pEnemy) continue;
                auto pTr = pEnemy->GetComponent<Engine::Transform>();
                if (!pTr) continue;
                const Engine::Vector3 e = pTr->GetPosition();
                const float dx = e.x - vAnchor.x;
                const float dz = e.z - vAnchor.z;
                const float t  = dx * vDir.x + dz * vDir.z;   // distance along the beam
                if (t < 0.f || t > kBeamLen) continue;
                const float px = dx - vDir.x * t;             // perpendicular offset
                const float pz = dz - vDir.z * t;
                if (px * px + pz * pz > fHalfW * fHalfW) continue;
                pEnemy->TakeDamage(iDmg, &e);
            }
        }

    private:
        std::shared_ptr<Engine::Transform> m_pTransform;
        Engine::Vector3 m_vColor{ 1.f, 0.2f, 0.4f };   // beam tint, 0..1 (set in Configure)

        int   m_iWeaponId = -1;
        int   m_iLevel    = 1;
        float m_fTickAcc  = 0.f;
        float m_fPhaseAcc = 0.f;     // duty-cycle phase timer
        bool  m_bOn       = true;    // starts firing; toggled by the duty cycle

        bool  m_bAimLock    = false; // lock heading per on-pulse (tower beams)
        bool  m_bYawLatched = false; // heading captured for the current pulse
        float m_fLockedYaw  = 0.f;   // latched heading while locked + on

        static constexpr float kBeamLen = 25.f;   // world units (full beam reach)
    };
}

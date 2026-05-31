#pragma once

#include "Scene/Scene.h"
#include "Core/Macro.h"
#include <cstdint>
#include <memory>

namespace Engine
{
    class Collider;
    class VoxelWorld;
    class Gauge;
    class Text;
}

namespace Client
{
    class EnemyCountHUD;
    class EnemySpawner;
    class Player;
    class TowerIntermissionUI;
    class TowerPlacementController;

    class GAME_DLL GameScene :
        public Engine::Scene
    {
    public:
        GameScene();
        virtual ~GameScene() override;

    private:
        // Phase V6 — Scene owns the voxel world. Player borrows a raw
        // pointer (Player::SetVoxelWorld) and calls into it for raycast
        // / break / place; lifetime is bound to the scene.
        std::unique_ptr<Engine::VoxelWorld> m_pVoxelWorld;

        // HP / XP gauges — Engine::Gauge Components on dedicated
        // GameObjects in the default layer (created in Init). The
        // Scene caches weak refs so Update can push the current ratio
        // (HP/MaxHP, Exp/XpToNext) and the resize-tracking pixel rect
        // each frame; the gauges' child UIRenderers self-register with
        // RenderManager via their own PreDraw.
        std::weak_ptr<Engine::Gauge>  m_pHPGauge;
        // XP gauge — sits just below the HP bar. Ratio pushed each frame
        // from Player::GetExp()/GetXpToNext().
        std::weak_ptr<Engine::Gauge>  m_pXPGauge;
        // Gold readout — replaces the old XP bar. Updated each frame from
        // Wallet::Money() (orbs now grant money, not XP).
        std::weak_ptr<Engine::Text>   m_pMoneyText;
        // Round survival countdown (top-centre). Shows seconds left to survive.
        std::weak_ptr<Engine::Text>   m_pRoundTimerText;

        // Installable-tower hotbar (bottom-right): one colour-coded icon per
        // tower type with the remaining count (owned - placed) on it. Index
        // 0 = attack (key 1), 1 = heal (key 2). Icons dim to grey at 0 left;
        // m_uTowerIconColor caches the last applied fill so Update only re-binds
        // the icon texture when the colour actually changes.
        static constexpr int          kTowerSlots = 2;
        std::weak_ptr<Engine::Gauge>  m_pTowerIcon[kTowerSlots];
        std::weak_ptr<Engine::Text>   m_pTowerCount[kTowerSlots];
        uint32_t                      m_uTowerIconColor[kTowerSlots] = { 0, 0 };

        std::weak_ptr<Player>         m_pPlayer;

        // Top-right debug HUD showing the current live enemy count as a
        // procedural 5x7 bitmap-font number. Scene-owned plain class
        // (draws a procedural digit atlas — different render path).
        // shared_ptr (not unique_ptr) so the per-frame UI render callback
        // can capture a weak_ptr and safely no-op after a scene change
        // frees this HUD mid-frame (between Update-register and Draw-run).
        std::shared_ptr<EnemyCountHUD> m_pEnemyCountHUD;

        // Periodic enemy spawning. Owns its own accumulator + round-robin
        // index so the scene's Update body stays a list of one-line
        // dispatches. Reads SpawnConfig (cadence / radius) and
        // EnemyDatabase (per-enemy stats) every tick.
        std::unique_ptr<EnemySpawner>  m_pEnemySpawner;

        // Current round number (the round the spawner is running, or — while
        // the intermission panel is up — the round just cleared). Drives the
        // Playing → Intermission → next-round loop in Update.
        int m_iRound = 0;

        // Brief post-round "collecting" phase: after surviving, the game keeps
        // running (not yet frozen) while leftover orbs magnet into the player,
        // then the shop opens. m_fCollectTimer caps the wait.
        bool  m_bCollectingOrbs = false;
        float m_fCollectTimer   = 0.f;

        // Between-round prep panel (weapon select + start) and the tower
        // placement controller. Weak refs: the GameObjects live in the
        // default layer; the scene keeps these to drive the round loop and to
        // register the placement ghost's ALPHA render callback each frame.
        std::weak_ptr<TowerIntermissionUI>     m_pIntermission;
        std::weak_ptr<TowerPlacementController> m_pPlacement;

    public:
        Engine::VoxelWorld* GetVoxelWorld() const { return m_pVoxelWorld.get(); }

    public:
        bool LoadSequences();
        bool CreateTexture();
        bool CreateSounds();
        bool CreateMesh();

    public:
        // Create one Enemy at a fixed spawn cell, set the player as its target.
        // Called by Player::Input on the N key.
        void SpawnEnemy();

    public:
        virtual bool Init() override;
        virtual void Update(float dt) override;
        virtual void Draw() override;
    };
}
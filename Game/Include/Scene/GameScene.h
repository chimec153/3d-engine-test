#pragma once

#include "Scene/Scene.h"
#include "Core/Macro.h"
#include <memory>

namespace Engine
{
    class Collider;
    class VoxelWorld;
}

namespace Client
{
    class EnemyCountHUD;

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

        // HPBar now lives as a UIControl-derived Component on a
        // dedicated GameObject in the default layer (created in Init).
        // Its child UIRenderers self-register with RenderManager via
        // their own PreDraw, so the Scene holds no direct reference
        // and re-registers nothing each frame.

        // Top-right debug HUD showing the current live enemy count as a
        // procedural 5x7 bitmap-font number. Scene-owned plain class
        // (draws a procedural digit atlas — different render path).
        std::unique_ptr<EnemyCountHUD> m_pEnemyCountHUD;

        // Periodic enemy spawning — drops an Enemy at a random angle
        // around the player every m_fEnemySpawnInterval seconds.
        float m_fEnemySpawnAcc      = 0.f;
        float m_fEnemySpawnInterval = 1.f;
        // Slow chase speed for testing; raise later for tuned gameplay.
        float m_fEnemyTestSpeed     = 1.0f;
        // Ring radius (cells) around the player where enemies materialise.
        float m_fEnemySpawnRadius   = 8.f;
        // Counter to alternate between Enemy mesh variants on each spawn.
        int   m_iEnemySpawnIdx      = 0;

    public:
        Engine::VoxelWorld* GetVoxelWorld() const { return m_pVoxelWorld.get(); }

    public:
        bool LoadSequences();
        bool CreateTexture();
        bool CreateSounds();
        bool CreateMesh();
        bool CreateTerrain();

    public:
        bool CreateMonster();

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
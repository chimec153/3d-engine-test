#pragma once

namespace Engine
{
    class Scene;
    class VoxelWorld;
}

namespace Client
{
    // Per-frame enemy spawning. Owns its own spawn accumulator and
    // round-robin index so the scene's Update body shrinks to a single
    // Tick(dt) call. Reads the spawn cadence and ring radius from
    // SpawnConfig and the per-enemy stats from EnemyDatabase on each
    // tick — so editing the CSVs at runtime would take effect on the
    // next spawn without a scene rebuild.
    class EnemySpawner
    {
    public:
        EnemySpawner(Engine::Scene* pScene, Engine::VoxelWorld* pWorld);

        void Tick(float fDeltaTime);

    private:
        Engine::Scene*       m_pScene = nullptr;
        Engine::VoxelWorld*  m_pWorld = nullptr;

        float m_fSpawnAcc = 0.f;
        // Round-robin index into EnemyDatabase rows — every variant in
        // enemies.csv shows up in turn.
        int   m_iSpawnIdx = 0;
    };
}

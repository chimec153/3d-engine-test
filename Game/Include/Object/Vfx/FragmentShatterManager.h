#pragma once
#include "Vector3.h"
#include "Core/Macro.h"   // dbg_new
#include <memory>
#include <vector>

namespace Engine
{
    class Mesh;
    class Material;
}

namespace Client
{
    // Death-shatter spawner. Holds the pre-fractured geometry (box_fragment.mesh
    // split into 12 shard meshes, each recentred on its own centroid) and, on an
    // enemy death, spawns 12 self-simulating FragmentShard GameObjects.
    //
    // CPU simulation + the normal render pipeline: each shard is an ordinary
    // GameObject drawn through the shared EnemyMeshRenderer path, so it inherits
    // the deferred toon shading + per-instance dissolve with no custom rendering,
    // no compute shader, and no hand-bound draw calls. (~hundreds of small rigid
    // pieces is negligible CPU cost.)
    //
    // Singleton mirroring VfxManager: GameScene calls Setup() once per session.
    class FragmentShatterManager
    {
        static FragmentShatterManager* m_pInst;

    public:
        static FragmentShatterManager* GetInst()
        {
            if (!m_pInst) m_pInst = dbg_new FragmentShatterManager;
            return m_pInst;
        }
        static void DestroyInst()
        {
            if (m_pInst) { delete m_pInst; m_pInst = nullptr; }
        }

        // Per-shape shard pool. Each variant loads <base>[N].mesh:
        //   BOX        → box_fragment[N].mesh        (enemy boxes)
        //   CAPSULE    → capsule_fragment[N].mesh    (enemy capsules)
        //   TOWER      → tower_fragment[N].mesh       (player turret)
        //   HEAL_TOWER → heal_tower_fragment[N].mesh  (heal cylinder)
        // Each variant may carry multiple bake variants (e.g. box_fragment.mesh,
        // box_fragment2.mesh, …) — SpawnShatter picks one at random per death so
        // bursts don't visually repeat.
        enum class VARIANT { BOX, CAPSULE, TOWER, HEAL_TOWER, COUNT };

        // Parse every fragment .mesh asset into recentred shard sets. Idempotent;
        // per-variant readiness is tracked separately (a missing capsule asset
        // doesn't disable the box variant and vice versa). Probes
        // box_fragment.mesh, box_fragment2.mesh, … up to kMaxBakes per variant.
        void Setup();

        // Burst the shards at a world position. eVariant picks which shard pool
        // (box vs capsule); fScale sizes the unit shards to the body; pMaterial
        // is the dying body's material (cloned + reused so shards match its
        // colour + toon shading); fGroundY is the floor the shards bounce/rest on.
        void SpawnShatter(VARIANT eVariant,
                          const Engine::Vector3& vPos, float fScale,
                          const std::shared_ptr<Engine::Material>& pMaterial,
                          float fGroundY);

    private:
        FragmentShatterManager() = default;
        ~FragmentShatterManager() = default;
        FragmentShatterManager(const FragmentShatterManager&) = delete;
        FragmentShatterManager& operator=(const FragmentShatterManager&) = delete;

        // Returns true if the asset produced at least one shard. iAssetIdx is
        // baked into the shard tag (FragmentShard_Box0_5 vs FragmentShard_Box1_5)
        // so distinct bakes never share a Mesh bindable.
        bool  LoadShards(VARIANT eVariant, int iAssetIdx, const char* pAssetPath);
        float Rand();   // [0,1) LCG

        struct Shard
        {
            std::shared_ptr<Engine::Mesh> mesh;     // single-container, centred on its centroid
            Engine::Vector3               centroid; // original offset within the unit box
        };

        static constexpr int kShards   = 12;
        static constexpr int kMaxBakes = 8;   // upper bound on box_fragmentN.mesh probe

        bool m_bSetupTried = false;
        // m_pools[variant] is a list of shard-sets, one per loaded .mesh bake.
        // SpawnShatter random-picks among the entries.
        std::vector<std::vector<Shard>> m_pools[(int)VARIANT::COUNT];
        unsigned int m_uSeed = 1u;
    };
}

#include "FragmentShatterManager.h"
#include "FragmentShard.h"

#include "Core/PathManager.h"
#include "Bindable/Mesh.h"
#include "Bindable/Material.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Layer.h"
#include "Types.h"

#include <cstdio>
#include <cmath>
#include <vector>

namespace Client
{
    FragmentShatterManager* FragmentShatterManager::m_pInst = nullptr;

    namespace
    {
        constexpr unsigned int kMeshMagic    = 0x4853454D;  // 'MESH'
        constexpr int          kVertexStride = 76;          // VertexStandard
    }

    float FragmentShatterManager::Rand()
    {
        m_uSeed = m_uSeed * 1664525u + 1013904223u;
        return static_cast<float>(m_uSeed >> 8) * (1.f / 16777216.f);
    }

    void FragmentShatterManager::Setup()
    {
        if (m_bSetupTried) return;
        m_bSetupTried = true;

        // Free the singleton at app shutdown (Client/Editor main calls
        // BindableRegistry::DestroyAll), mirroring VfxManager — otherwise the
        // dbg_new in GetInst is reported as a leak.
        Engine::BindableRegistry::Register([]() { FragmentShatterManager::DestroyInst(); });

        // Probe every supported asset index for both variants. i=0 maps to the
        // unsuffixed name (box_fragment.mesh) and i>=1 to numbered siblings
        // (box_fragment2.mesh, box_fragment3.mesh, …). Missing indices are
        // skipped silently — the loop doesn't early-out, so non-contiguous
        // numbering (e.g. 1 and 3 with no 2) still loads what exists.
        for (int v = 0; v < (int)VARIANT::COUNT; ++v)
        {
            const char* pBase =
                (v == (int)VARIANT::CAPSULE)    ? "capsule_fragment"    :
                (v == (int)VARIANT::TOWER)      ? "tower_fragment"      :
                (v == (int)VARIANT::HEAL_TOWER) ? "heal_tower_fragment" :
                                                  "box_fragment";
            for (int i = 0; i < kMaxBakes; ++i)
            {
                char szAsset[64];
                if (i == 0)
                    std::snprintf(szAsset, sizeof(szAsset), "/Game/Mesh/%s.mesh", pBase);
                else
                    std::snprintf(szAsset, sizeof(szAsset), "/Game/Mesh/%s%d.mesh", pBase, i + 1);
                LoadShards((VARIANT)v, i, szAsset);
            }
        }
    }

    bool FragmentShatterManager::LoadShards(VARIANT eVariant, int iAssetIdx, const char* pAssetPath)
    {
        const char* pKind =
            (eVariant == VARIANT::CAPSULE)    ? "Cap"  :
            (eVariant == VARIANT::TOWER)      ? "Twr"  :
            (eVariant == VARIANT::HEAL_TOWER) ? "Heal" :
                                                "Box";

        char szPath[MAX_PATH] = {};
        Engine::CPathManager::GetInst()->ResolveMB(pAssetPath, MESH_PATH, szPath);

        FILE* f = nullptr;
        fopen_s(&f, szPath, "rb");
        if (!f) return false;

        unsigned int magic = 0, version = 0;
        int containerCount = 0;
        fread(&magic, 4, 1, f);
        fread(&version, 4, 1, f);
        if (magic != kMeshMagic || version != 2) { fclose(f); return false; }
        fread(&containerCount, 4, 1, f);

        std::vector<Shard> shards;
        shards.reserve(kShards);

        for (int c = 0; c < containerCount; ++c)
        {
            int vc = 0;
            fread(&vc, 4, 1, f);
            std::vector<Engine::VertexStandard> verts(vc > 0 ? vc : 0);
            if (vc > 0) fread(verts.data(), sizeof(Engine::VertexStandard), vc, f);

            short ibc = 0;
            fread(&ibc, 2, 1, f);
            std::vector<unsigned int> indices;
            for (short j = 0; j < ibc; ++j)
            {
                int sub = 0;
                fread(&sub, 4, 1, f);
                if (sub > 0)
                {
                    size_t base = indices.size();
                    indices.resize(base + sub);
                    fread(indices.data() + base, 4, sub, f);
                }
            }

            int matCount = 0;
            fread(&matCount, 4, 1, f);
            for (int m = 0; m < matCount; ++m)
            {
                int tagLen = 0;
                fread(&tagLen, 4, 1, f);
                if (tagLen > 0) fseek(f, tagLen, SEEK_CUR);
            }

            if (c >= kShards || vc == 0 || indices.empty())
                continue;

            // Centroid = average vertex position; recentre the shard on it so
            // the object's Transform rotates about the shard's own middle and
            // the centroid doubles as the outward spawn offset.
            Engine::Vector3 cen(0.f, 0.f, 0.f);
            for (const auto& v : verts) { cen.x += v.pos.x; cen.y += v.pos.y; cen.z += v.pos.z; }
            cen = cen / static_cast<float>(vc);
            for (auto& v : verts)
            {
                v.pos.x -= cen.x; v.pos.y -= cen.y; v.pos.z -= cen.z;
                // The baker gives every triangle the same trivial UVs, which makes
                // the dissolve noise repeat identically on every facet. Derive a
                // position-based UV (axes mixed so opposite faces don't collide)
                // so the dissolve varies across the whole shard surface.
                v.uv.x = v.pos.x * 2.f + v.pos.y;
                v.uv.y = v.pos.z * 2.f + v.pos.y;
            }

            // Unique tag per (variant, asset-index, shard-index) → its own
            // instancing bucket. Avoids the shared-geometry instancing hazard
            // (one mesh stamped on all) AND keeps box/capsule shards AND
            // distinct bakes (box_fragment vs box_fragment2) in separate
            // buckets.
            char szTag[48];
            std::snprintf(szTag, sizeof(szTag), "FragmentShard_%s%d_%d", pKind, iAssetIdx, c);
            auto pMesh = Engine::StaticCreateBindable<Engine::Mesh>(szTag, verts, indices);

            Shard s;
            s.mesh     = pMesh;
            s.centroid = cen;
            shards.push_back(std::move(s));
        }

        fclose(f);
        if (shards.empty()) return false;
        m_pools[(int)eVariant].push_back(std::move(shards));
        return true;
    }

    void FragmentShatterManager::SpawnShatter(VARIANT eVariant,
                                              const Engine::Vector3& vPos, float fScale,
                                              const std::shared_ptr<Engine::Material>& pMaterial,
                                              float fGroundY)
    {
        const auto& pools = m_pools[(int)eVariant];
        if (pools.empty()) return;

        // Random-pick which bake to burst this death. Rand() ∈ [0,1) so the
        // truncation is unbiased across the pool count.
        const int iPick = static_cast<int>(Rand() * static_cast<float>(pools.size()));
        const std::vector<Shard>& shards = pools[iPick];

        auto* pScene = Engine::SceneManager::GetInst()->GetScene();
        if (!pScene) return;
        auto pLayer = pScene->FindLayer(DEFAULT_LAYER);
        if (!pLayer) return;

        // Clone the body material once so all 12 shards share it (and don't
        // mutate the enemy's), inheriting its colour + toon shading model.
        std::shared_ptr<Engine::Material> pMat = pMaterial
            ? std::static_pointer_cast<Engine::Material>(pMaterial->Clone())
            : nullptr;

        // The killing blow set the enemy's hit-flash to full white (TakeDamage),
        // and the enemy decays it each frame — but our clone froze it at 1, so
        // EnemyPSInst would lerp every shard to solid white. Clear it.
        if (pMat)
        {
            pMat->SetHitFlash(Engine::Vector3(0.f, 0.f, 0.f), 0.f);
            pMat->UsePaperBurn();   // enable the solo EnemyPS dissolve branch
        }

        for (const Shard& s : shards)
        {
            if (!s.mesh) continue;

            auto pObj = pScene->CreateGameObject<FragmentShard>("FragShard", pLayer);
            if (!pObj) continue;

            const Engine::Vector3 vWorld = vPos + s.centroid * fScale;

            // Outward (from the body centre, through the shard) + upward bias.
            Engine::Vector3 dir = s.centroid;
            float len = dir.Length();
            dir = (len > 1e-4f) ? dir / len : Engine::Vector3(0.f, 1.f, 0.f);

            Engine::Vector3 vel =
                  dir * (5.f + Rand() * 3.f)
                + Engine::Vector3(0.f, 4.5f, 0.f)
                + Engine::Vector3((Rand() - 0.5f) * 4.f, (Rand() - 0.5f) * 2.f, (Rand() - 0.5f) * 4.f);

            Engine::Vector3 angVel((Rand() - 0.5f) * 24.f, (Rand() - 0.5f) * 24.f, (Rand() - 0.5f) * 24.f);

            pObj->Launch(s.mesh, pMat, vWorld, vel, angVel, fScale, 1.6f, fGroundY);
        }
    }
}

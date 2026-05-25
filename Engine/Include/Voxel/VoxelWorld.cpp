#include "VoxelWorld.h"
#include "VoxelMesher.h"
#include "../Core/Macro.h"
#include "../Bindable/Mesh.h"
#include <cmath>
#include <limits>
#include "../Bindable/Transform.h"
#include "../Bindable/InputLayout.h"
#include "../Bindable/Topology.h"
#include "../Bindable/VertexShader.h"
#include "../Bindable/PixelShader.h"
#include "../Bindable/Material.h"
#include "../Bindable/BindableManager.h"
#include "../Component/MeshRendererComponent.h"
#include "../GameObject/GameObject.h"
#include "../Scene/Scene.h"
#include "../Scene/Layer.h"
#include <string>

namespace Engine
{
    namespace
    {
        // Integer floor-division and floor-mod. Plain `/` and `%` truncate
        // toward zero in C++, so wx = -1, kSize = 16 would give chunk 0
        // and local -1 — both wrong. We want chunk -1, local 15.
        constexpr int N = VoxelChunk::kSize;

        inline int FloorDivN(int a)
        {
            int q = a / N;
            if ((a % N != 0) && (a < 0)) --q;
            return q;
        }

        inline int FloorModN(int a)
        {
            int m = a % N;
            if (m < 0) m += N;
            return m;
        }
    }

    VoxelWorld::VoxelWorld(Scene* pScene, const std::shared_ptr<Layer>& pLayer)
        : m_pScene(pScene)
        , m_pLayer(pLayer)
    {
    }

    VoxelWorld::~VoxelWorld() = default;

    VoxelWorld::ChunkCoord VoxelWorld::WorldToChunk(int wx, int wy, int wz)
    {
        return { FloorDivN(wx), FloorDivN(wy), FloorDivN(wz) };
    }

    void VoxelWorld::WorldToLocal(int wx, int wy, int wz, int& lx, int& ly, int& lz)
    {
        lx = FloorModN(wx);
        ly = FloorModN(wy);
        lz = FloorModN(wz);
    }

    VoxelWorld::ChunkRecord* VoxelWorld::FindChunk(const ChunkCoord& c)
    {
        auto it = m_chunks.find(c);
        return it == m_chunks.end() ? nullptr : &it->second;
    }

    const VoxelWorld::ChunkRecord* VoxelWorld::FindChunk(const ChunkCoord& c) const
    {
        auto it = m_chunks.find(c);
        return it == m_chunks.end() ? nullptr : &it->second;
    }

    VoxelWorld::ChunkRecord* VoxelWorld::EnsureChunk(const ChunkCoord& c)
    {
        if (auto* p = FindChunk(c)) return p;
        if (!m_pScene || !m_pLayer) return nullptr;

        ChunkRecord rec;
        rec.pChunk = std::make_unique<VoxelChunk>();

        const auto [cx, cy, cz] = c;
        std::string tag = "VoxelChunk_"
            + std::to_string(cx) + "_"
            + std::to_string(cy) + "_"
            + std::to_string(cz);

        rec.pObj = m_pScene->CreateGameObject<GameObject>(tag, m_pLayer);
        if (!rec.pObj) return nullptr;

        if (auto pTr = rec.pObj->AddComponent<Transform>("transform"))
        {
            pTr->SetPosition(
                static_cast<float>(cx * N),
                static_cast<float>(cy * N),
                static_cast<float>(cz * N));
        }

        rec.pMR = rec.pObj->AddComponent<MeshRendererComponent>("mesh_renderer");
        if (rec.pMR)
        {
            rec.pMR->AddBindable(StaticFindBindable<InputLayout>("Standard"));
            rec.pMR->AddBindable(StaticFindBindable<Topology>("TriangleList"));
            rec.pMR->SetVertexShader(StaticFindBindable<VertexShader>(STANDARD_VS));
            // Voxel chunks have no per-block textures/Material yet, so
            // the default STANDARD_PS (samples diffuse/normal/spec) draws
            // black. Use the texture-less variant — same NavDebug uses.
            rec.pMR->SetPixelShader (StaticFindBindable<PixelShader> (STANDARD_SOLID_PS));

            // PS_NoDiffuse... pulls colour from the material ConstantBuffer
            // (g_vDiffuseColor). Without an explicit material the CB keeps
            // whatever the previously-drawn mesh wrote — which produced
            // garbage colours after the HUD pass started writing the CB.
            // One shared default-white material across every chunk is
            // enough; per-block colouring can replace this later.
            auto pVoxelMat = StaticFindBindable<Material>("VoxelMaterial");
            if (!pVoxelMat)
            {
                pVoxelMat = StaticCreateBindable<Material>("VoxelMaterial");
                if (pVoxelMat)
                {
                    pVoxelMat->SetDiffuseColor(0.3f, 0.6f, 0.3f, 1.f);
                    pVoxelMat->SetEmissiveColor({ 0.f, 0.f, 0.f, 0.f });
                    // 캐릭터·복셀 모두 Toon — 단일 공유 머티리얼이라 setter 한 번이면
                    // 모든 청크에 적용. Enemy처럼 per-instance Clone이 필요 없음(hit
                    // flash 같은 인스턴스 파라미터를 쓰지 않으므로 인스턴싱 유지).
                    pVoxelMat->SetShadingModel(Engine::SHADING_MODEL_TOON);
                }
            }
            if (pVoxelMat) rec.pMR->SetMaterial(pVoxelMat);
        }

        auto [it, ok] = m_chunks.emplace(c, std::move(rec));
        return &it->second;
    }

    BlockType VoxelWorld::GetBlock(int wx, int wy, int wz) const
    {
        const ChunkCoord cc = WorldToChunk(wx, wy, wz);
        const ChunkRecord* rec = FindChunk(cc);
        if (!rec || !rec->pChunk) return BlockType::Air;

        int lx, ly, lz;
        WorldToLocal(wx, wy, wz, lx, ly, lz);
        return rec->pChunk->GetBlock(lx, ly, lz);
    }

    void VoxelWorld::SetBlock(int wx, int wy, int wz, BlockType type)
    {
        const ChunkCoord cc = WorldToChunk(wx, wy, wz);
        int lx, ly, lz;
        WorldToLocal(wx, wy, wz, lx, ly, lz);

        ChunkRecord* rec = EnsureChunk(cc);
        if (!rec || !rec->pChunk) return;

        if (rec->pChunk->GetBlock(lx, ly, lz) == type) return;
        rec->pChunk->SetBlock(lx, ly, lz, type);

        RebuildChunkMesh(cc);

        // If the edit touched a chunk border, the neighbour chunk's mesh
        // may have a now-exposed (or now-hidden) face along the shared
        // boundary. Re-mesh existing neighbours — don't auto-create them.
        const auto remeshIfExists = [&](int dx, int dy, int dz)
        {
            ChunkCoord nc{ std::get<0>(cc) + dx,
                            std::get<1>(cc) + dy,
                            std::get<2>(cc) + dz };
            if (FindChunk(nc)) RebuildChunkMesh(nc);
        };

        if (lx == 0)     remeshIfExists(-1,  0,  0);
        if (lx == N - 1) remeshIfExists(+1,  0,  0);
        if (ly == 0)     remeshIfExists( 0, -1,  0);
        if (ly == N - 1) remeshIfExists( 0, +1,  0);
        if (lz == 0)     remeshIfExists( 0,  0, -1);
        if (lz == N - 1) remeshIfExists( 0,  0, +1);
    }

    int VoxelWorld::GetSurfaceHeight(int wx, int wz, int yMin, int yMax) const
    {
        for (int y = yMax; y >= yMin; --y)
        {
            if (IsSolid(GetBlock(wx, y, wz)))
                return y;
        }
        return yMin - 1;
    }

    VoxelRaycastHit VoxelWorld::Raycast(const Vector3& origin,
                                       const Vector3& dir,
                                       float maxDist) const
    {
        VoxelRaycastHit out = {};
        out.hit = false;

        Vector3 d = dir;
        float dlen = d.Length();
        if (dlen < 1e-6f) return out;
        d /= dlen;

        int x = static_cast<int>(std::floor(origin.x));
        int y = static_cast<int>(std::floor(origin.y));
        int z = static_cast<int>(std::floor(origin.z));

        const int stepX = (d.x > 0.f) ? 1 : ((d.x < 0.f) ? -1 : 0);
        const int stepY = (d.y > 0.f) ? 1 : ((d.y < 0.f) ? -1 : 0);
        const int stepZ = (d.z > 0.f) ? 1 : ((d.z < 0.f) ? -1 : 0);

        const float INF = std::numeric_limits<float>::infinity();

        auto tToBoundary = [](float o, float d, int step) -> float
        {
            if (step == 0) return INF;
            float edge = (step > 0) ? std::floor(o) + 1.0f : std::floor(o);
            return (edge - o) / d;
        };

        float tMaxX = tToBoundary(origin.x, d.x, stepX);
        float tMaxY = tToBoundary(origin.y, d.y, stepY);
        float tMaxZ = tToBoundary(origin.z, d.z, stepZ);

        const float tDeltaX = (stepX == 0) ? INF : std::fabs(1.0f / d.x);
        const float tDeltaY = (stepY == 0) ? INF : std::fabs(1.0f / d.y);
        const float tDeltaZ = (stepZ == 0) ? INF : std::fabs(1.0f / d.z);

        int faceX = 0, faceY = 0, faceZ = 0;  // face normal of the last crossed boundary
        float t = 0.f;

        while (t <= maxDist)
        {
            if (IsSolid(GetBlock(x, y, z)))
            {
                out.hit = true;
                out.blockX = x; out.blockY = y; out.blockZ = z;
                out.faceX = faceX; out.faceY = faceY; out.faceZ = faceZ;
                out.distance = t;
                return out;
            }

            if (tMaxX < tMaxY && tMaxX < tMaxZ)
            {
                t = tMaxX;
                tMaxX += tDeltaX;
                x += stepX;
                faceX = -stepX; faceY = 0; faceZ = 0;
            }
            else if (tMaxY < tMaxZ)
            {
                t = tMaxY;
                tMaxY += tDeltaY;
                y += stepY;
                faceX = 0; faceY = -stepY; faceZ = 0;
            }
            else
            {
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                z += stepZ;
                faceX = 0; faceY = 0; faceZ = -stepZ;
            }
        }

        return out;
    }

    void VoxelWorld::RebuildChunkMesh(const ChunkCoord& c)
    {
        ChunkRecord* rec = FindChunk(c);
        if (!rec || !rec->pMR || !rec->pChunk) return;

        const int cx = std::get<0>(c);
        const int cy = std::get<1>(c);
        const int cz = std::get<2>(c);

        // Cross-chunk lookup: caller asks for a local-space coord that's
        // outside [0, N). Translate to world and route through the
        // World's own GetBlock so an unloaded neighbour still reads Air.
        VoxelMesher::NeighbourLookup lookup =
            [this, cx, cy, cz](int lx, int ly, int lz) -> BlockType
        {
            return GetBlock(cx * N + lx, cy * N + ly, cz * N + lz);
        };

        std::vector<VertexStandard> verts;
        std::vector<unsigned int>   inds;
        VoxelMesher::Build(*rec->pChunk, verts, inds, lookup);

        std::shared_ptr<Mesh> pMesh;
        if (!verts.empty() && !inds.empty())
        {
            pMesh = std::make_shared<Mesh>(verts, inds);
            // Per-chunk unique tag: MeshRendererComponent::UpdateInstanceKey
            // hashes Mesh->GetTag(). Without a unique tag every chunk would
            // hash to the same key and land in one instance bucket, where
            // TryRenderInstancedBucket draws only pFirst->GetMesh() for the
            // whole bucket — copying chunk 0's geometry to every chunk.
            pMesh->SetTag("VoxelChunkMesh_"
                + std::to_string(cx) + "_"
                + std::to_string(cy) + "_"
                + std::to_string(cz));
        }
        rec->pMR->SetMesh(pMesh);
    }
}

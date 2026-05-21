#pragma once
#include "../Core/Macro.h"
#include "../Types.h"
#include "BlockType.h"
#include <functional>
#include <memory>
#include <vector>

namespace Engine
{
    class VoxelChunk;
    class Mesh;

    namespace VoxelMesher
    {
        // Phase V5 — out-of-chunk neighbour lookup, used for cross-chunk
        // face culling. Called with local-space coords that fall OUTSIDE
        // [0, kSize). Return Air to keep the face, return Solid to cull
        // it. Empty (default) means "treat outside as Air" (the V3/V4
        // behaviour).
        using NeighbourLookup = std::function<BlockType(int /*lx*/, int /*ly*/, int /*lz*/)>;

        // Phase V2 — neighbour-aware mesh build (face culling enabled
        // from V3 onward). Output is APPENDED to the supplied buffers —
        // caller is responsible for clearing if a fresh build is wanted.
        ENGINE_DLL void Build(const VoxelChunk& chunk,
                              std::vector<VertexStandard>& outVertices,
                              std::vector<unsigned int>& outIndices,
                              const NeighbourLookup& neighbour = {});

        // Phase V4 — one-call rebuild helper. Returns a freshly-built
        // Mesh ready to hand to MeshRendererComponent::SetMesh. Use
        // this from gameplay code that mutates the chunk and wants the
        // GPU to follow:
        //     chunk.SetBlock(x,y,z, BlockType::Air);
        //     pMR->SetMesh(VoxelMesher::BuildMesh(chunk));
        ENGINE_DLL std::shared_ptr<Mesh> BuildMesh(const VoxelChunk& chunk);
    }
}

#pragma once
#include "../Core/Macro.h"
#include "BlockType.h"
#include <array>

namespace Engine
{
    // Phase V1 — dumb data container: a 16x16x16 grid of BlockType.
    // Meshing, neighbour-aware face culling, and Mesh/MeshRenderer
    // wiring are added in later phases.
    class ENGINE_DLL VoxelChunk
    {
    public:
        static constexpr int kSize   = 16;
        static constexpr int kVolume = kSize * kSize * kSize;

        VoxelChunk();

        BlockType GetBlock(int x, int y, int z) const;
        void      SetBlock(int x, int y, int z, BlockType type);

        bool InBounds(int x, int y, int z) const;

        // Block-out-of-bounds reads return Air so callers (later: the
        // mesher) can treat anything outside the chunk as empty without
        // branching at every neighbour lookup.
        BlockType GetBlockSafe(int x, int y, int z) const;

    private:
        static int Index(int x, int y, int z)
        {
            return x + y * kSize + z * kSize * kSize;
        }

        std::array<BlockType, kVolume> m_blocks;
    };
}

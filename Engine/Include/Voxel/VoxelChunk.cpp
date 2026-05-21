#include "VoxelChunk.h"
#include <cassert>

namespace Engine
{
    VoxelChunk::VoxelChunk()
    {
        m_blocks.fill(BlockType::Air);
    }

    bool VoxelChunk::InBounds(int x, int y, int z) const
    {
        return x >= 0 && x < kSize
            && y >= 0 && y < kSize
            && z >= 0 && z < kSize;
    }

    BlockType VoxelChunk::GetBlock(int x, int y, int z) const
    {
        assert(InBounds(x, y, z));
        return m_blocks[Index(x, y, z)];
    }

    void VoxelChunk::SetBlock(int x, int y, int z, BlockType type)
    {
        assert(InBounds(x, y, z));
        m_blocks[Index(x, y, z)] = type;
    }

    BlockType VoxelChunk::GetBlockSafe(int x, int y, int z) const
    {
        if (!InBounds(x, y, z))
            return BlockType::Air;
        return m_blocks[Index(x, y, z)];
    }
}

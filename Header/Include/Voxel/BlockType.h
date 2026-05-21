#pragma once
#include <cstdint>

namespace Engine
{
    enum class BlockType : uint8_t
    {
        Air   = 0,
        Stone = 1,
    };

    inline bool IsSolid(BlockType t)
    {
        return t != BlockType::Air;
    }

    // Seconds an enemy takes to break the given block. Negative for blocks
    // that can't (or don't need to) be broken — Air, etc. Extend with new
    // case entries when more block types are added.
    inline float BlockBreakTime(BlockType t)
    {
        switch (t)
        {
        case BlockType::Stone: return 3.0f;
        default:               return -1.0f;
        }
    }
}

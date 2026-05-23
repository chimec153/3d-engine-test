#pragma once
#include <vector>

namespace Engine
{
    class VoxelWorld;
}

namespace Client
{
    // 2D grid A* for the tower-defense enemy. All blocks live on a single
    // wall layer (Client::kWallY) — solid = impassable wall, air = walkable
    // floor. Entering a solid cell adds BlockBreakTime to its edge cost,
    // so A* naturally picks "break the wall if it's faster than going
    // around." Move time per cell is 1/fSpeed.
    namespace Pathfinder
    {
        struct PathStep
        {
            int  x, z;        // world cell xz
            bool bBreak;      // entering this cell needs to break a wall first
        };

        // Returns the step-by-step path from start (exclusive) to end (inclusive).
        // False on failure. xz search is clamped to [start +/- iSearchRange].
        bool FindPath(const Engine::VoxelWorld& world,
                      int sx, int sz,
                      int ex, int ez,
                      float fSpeed,
                      int iSearchRange,
                      std::vector<PathStep>& outPath);
    }
}

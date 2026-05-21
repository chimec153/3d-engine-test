#pragma once
#include <vector>

namespace Engine
{
    class VoxelWorld;
}

namespace Client
{
    // Grid A* for the tower-defense enemy. Assumes planar movement on a fixed
    // y (normally 1, one above the floor). Move time per cell is 1/fSpeed.
    // Entering a solid cell adds BlockBreakTime to its edge cost — that's how
    // A* naturally prefers "break the wall if it's faster than going around."
    namespace Pathfinder
    {
        struct PathStep
        {
            int  x, y, z;     // world block coordinate of the cell
            bool bBreak;      // entering this cell needs to break a solid block first
        };

        // Returns the step-by-step path from start (exclusive) to end (inclusive).
        // False on failure. xz search is clamped to [start +/- iSearchRange].
        bool FindPath(const Engine::VoxelWorld& world,
                      int sx, int sy, int sz,
                      int ex, int ey, int ez,
                      float fSpeed,
                      int iSearchRange,
                      std::vector<PathStep>& outPath);
    }
}

#pragma once
#include "Vector3.h"
#include <vector>

namespace Engine
{
    class VoxelWorld;
}

namespace Client
{
    // Shared 2D flow field for the tower-defense enemies. One field per
    // goal cell (typically the player); every enemy samples it to know
    // which neighbour to head toward, so per-enemy A* drops to a single
    // Dijkstra rebuild on goal-cell change.
    //
    // 8-neighbour pass with break-cost on solid cells: orthogonal cost =
    // 1, diagonal = sqrt(2), and entering a breakable wall adds its
    // BlockBreakTime. Diagonals are forbidden when either orthogonal
    // neighbour is solid (corner-cut prevention), so the recorded
    // direction always traces a clear lane.
    //
    // Sampling outside the field window returns false — callers should
    // fall back to straight-line steering until the field rebuilds with
    // the new goal centre.
    class FlowField
    {
    public:
        // Half-extent of the square window around the goal cell. 64 is the
        // same search radius the legacy A* used, kept identical so spawn
        // pockets stay reachable.
        static constexpr int kRadius = 64;
        static constexpr int kSide   = kRadius * 2 + 1;

        FlowField();

        // Rebuild the field centred on (gx, gz) with the given world. No-op
        // when the centre matches the last build (use ForceRebuild for that).
        // Returns true if a rebuild ran.
        bool Rebuild(const Engine::VoxelWorld& world, int gx, int gz);
        bool ForceRebuild(const Engine::VoxelWorld& world, int gx, int gz);

        // Last-built goal cell.
        int GoalX() const { return m_iGoalX; }
        int GoalZ() const { return m_iGoalZ; }
        bool HasGoal() const { return m_bHasGoal; }

        // Sample at (cx, cz). On success writes the recommended next cell
        // and a unit direction vector toward it (xz plane, y=0). Returns
        // false when the cell is outside the window or has no recorded
        // direction (unreachable / is the goal itself).
        bool Sample(int cx, int cz,
                    int& outNextX, int& outNextZ,
                    Engine::Vector3& outDir) const;

    private:
        // Row-major [kSide * kSide]. Index = (cz - originZ) * kSide + (cx - originX).
        std::vector<float>   m_vG;        // cost-to-goal; +inf for unreached
        std::vector<int8_t>  m_vDir;      // 0..7 = neighbour index, -1 = none

        int  m_iGoalX   = 0;
        int  m_iGoalZ   = 0;
        int  m_iOriginX = 0;             // world cell at grid[0,0]
        int  m_iOriginZ = 0;
        bool m_bHasGoal = false;

        // grid-space index from world cell, -1 if outside.
        int  Index(int cx, int cz) const;
    };
}

#pragma once
#include "Vector3.h"
#include <cstdint>
#include <utility>
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

        // Rebuild the field centred on (gx, gz) with the given world.
        // vecBlocked lists extra (cell_x, cell_z) cells the spawner has marked
        // impassable (player-placed towers) — voxel may still report Air there
        // but pathing treats them as unbreakable walls. The goal cell itself is
        // exempt (so an enemy can chase a tower target). No-op when both the
        // centre and the blocked-set fingerprint match the last build (use
        // ForceRebuild for an unconditional rebuild). Returns true if a rebuild
        // actually ran.
        bool Rebuild(const Engine::VoxelWorld& world, int gx, int gz,
                     const std::vector<std::pair<int, int>>& vecBlocked = {});
        bool ForceRebuild(const Engine::VoxelWorld& world, int gx, int gz,
                          const std::vector<std::pair<int, int>>& vecBlocked = {});

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

        // True if (cx, cz) was in the blocked-cell set at the last rebuild.
        // Enemies consult this after Sample so they don't step into a tower
        // cell when the tower itself is the aggro goal — Dijkstra still
        // records a direction toward the goal (so they face it for melee),
        // but the cell must be treated like a wall on the way in.
        bool IsBlocked(int cx, int cz) const;

        // True if the last-built field actually reached (cx, cz) — i.e. there
        // is a finite-cost path from this cell to the goal. The spawner uses
        // this to detect a goal that is walled off (e.g. a tower aggro target
        // boxed in by other towers, or a goal cell whose voxel was solid so
        // the field never expanded): when the goal can't be reached from the
        // player's area, every enemy would freeze, so it retargets instead.
        bool Reaches(int cx, int cz) const;

    private:
        // Row-major [kSide * kSide]. Index = (cz - originZ) * kSide + (cx - originX).
        std::vector<float>   m_vG;        // cost-to-goal; +inf for unreached
        std::vector<int8_t>  m_vDir;      // 0..7 = neighbour index, -1 = none

        int  m_iGoalX   = 0;
        int  m_iGoalZ   = 0;
        int  m_iOriginX = 0;             // world cell at grid[0,0]
        int  m_iOriginZ = 0;
        bool m_bHasGoal = false;

        // Fingerprint of the last-built blocked-cell set. Order-independent
        // hash so the spawner can hand us an unsorted list without forcing a
        // pointless rebuild on shuffled order.
        uint64_t m_uBlockedFingerprint = 0;

        // Last-rebuilt blocked-cell list, kept so IsBlocked() can answer
        // per-cell queries from enemies. Small (tens of towers at most), so
        // a linear scan is fine and beats a 16k-entry side bitmap.
        std::vector<std::pair<int, int>> m_vBlocked;

        // grid-space index from world cell, -1 if outside.
        int  Index(int cx, int cz) const;

        // Order-independent hash of (cell_x, cell_z) pairs — combined via XOR
        // of per-cell mixes so list order doesn't affect the result.
        static uint64_t HashBlocked(const std::vector<std::pair<int, int>>& v);
    };
}

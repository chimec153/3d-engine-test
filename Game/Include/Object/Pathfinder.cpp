#include "Pathfinder.h"
#include "../GameDefs.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include <cmath>
#include <queue>
#include <unordered_map>
#include <limits>
#include <cstdint>

namespace Client
{
    namespace Pathfinder
    {
        namespace
        {
            // Pack (x, z) into a 64-bit key. Coordinates are offset to keep
            // them non-negative; A* only explores a bounded box around the
            // start, so 32 bits per axis is plenty of headroom.
            inline uint64_t PackKey(int x, int z)
            {
                const uint64_t ux = static_cast<uint64_t>(x + (1 << 30)) & 0xFFFFFFFFULL;
                const uint64_t uz = static_cast<uint64_t>(z + (1 << 30)) & 0xFFFFFFFFULL;
                return (ux << 32) | uz;
            }

            struct NodeInfo
            {
                float    fG;            // accumulated cost (seconds)
                int      iPrevX, iPrevZ;
                bool     bClosed;
                bool     bEnterBreak;   // entering this cell required breaking a wall
            };

            struct OpenEntry
            {
                float fF;        // g + h
                int   x, z;
                bool  operator<(const OpenEntry& o) const { return fF > o.fF; }   // min-heap
            };
        }

        bool FindPath(const Engine::VoxelWorld& world,
                      int sx, int sz,
                      int ex, int ez,
                      float fSpeed,
                      int iSearchRange,
                      std::vector<PathStep>& outPath)
        {
            outPath.clear();
            if (fSpeed <= 0.f) return false;

            // The start cell itself can't be inside a wall — if it is, the
            // enemy is stuck and there is no path to anywhere.
            if (Engine::IsSolid(world.GetBlock(sx, kWallY, sz))) return false;

            const float fMoveCost = 1.f / fSpeed;

            std::priority_queue<OpenEntry> open;
            std::unordered_map<uint64_t, NodeInfo> visited;

            const auto heuristic = [&](int x, int z) {
                return (std::abs(ex - x) + std::abs(ez - z)) * fMoveCost;
            };

            visited[PackKey(sx, sz)] = { 0.f, sx, sz, false, false };
            open.push({ heuristic(sx, sz), sx, sz });

            const int dx[4] = { 1, -1, 0,  0 };
            const int dz[4] = { 0,  0, 1, -1 };

            while (!open.empty())
            {
                const OpenEntry cur = open.top(); open.pop();

                auto itCur = visited.find(PackKey(cur.x, cur.z));
                if (itCur == visited.end()) continue;
                if (itCur->second.bClosed)  continue;
                itCur->second.bClosed = true;

                if (cur.x == ex && cur.z == ez)
                {
                    std::vector<PathStep> rev;
                    int cx = cur.x, cz = cur.z;
                    while (cx != sx || cz != sz)
                    {
                        auto it = visited.find(PackKey(cx, cz));
                        if (it == visited.end()) return false;
                        rev.push_back({ cx, cz, it->second.bEnterBreak });
                        const int px = it->second.iPrevX;
                        const int pz = it->second.iPrevZ;
                        cx = px; cz = pz;
                    }
                    outPath.assign(rev.rbegin(), rev.rend());
                    return true;
                }

                if (std::abs(cur.x - sx) > iSearchRange ||
                    std::abs(cur.z - sz) > iSearchRange)
                {
                    continue;
                }

                const float gCur = itCur->second.fG;

                for (int i = 0; i < 4; ++i)
                {
                    const int nx = cur.x + dx[i];
                    const int nz = cur.z + dz[i];

                    // 2D transition: enter (nx, nz) on the wall layer.
                    //   air  → flat walk, cost = fMoveCost
                    //   wall → break first, cost = fMoveCost + BlockBreakTime
                    //   unbreakable → skip
                    const Engine::BlockType bSide = world.GetBlock(nx, kWallY, nz);
                    const bool bSolid = Engine::IsSolid(bSide);

                    bool  bBreakHere = false;
                    float fBreakCost = 0.f;
                    if (bSolid)
                    {
                        const float fBreak = Engine::BlockBreakTime(bSide);
                        if (fBreak < 0.f) continue;   // unbreakable wall
                        bBreakHere = true;
                        fBreakCost = fBreak;
                    }

                    const float fEnterCost = fMoveCost + fBreakCost;
                    const float gNext = gCur + fEnterCost;
                    const uint64_t k = PackKey(nx, nz);
                    auto it = visited.find(k);
                    if (it != visited.end())
                    {
                        if (it->second.bClosed)   continue;
                        if (gNext >= it->second.fG) continue;
                        it->second.fG          = gNext;
                        it->second.iPrevX      = cur.x;
                        it->second.iPrevZ      = cur.z;
                        it->second.bEnterBreak = bBreakHere;
                    }
                    else
                    {
                        visited[k] = { gNext, cur.x, cur.z, false, bBreakHere };
                    }
                    open.push({ gNext + heuristic(nx, nz), nx, nz });
                }
            }

            return false;
        }
    }
}

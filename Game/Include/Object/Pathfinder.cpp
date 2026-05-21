#include "Pathfinder.h"
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
            // Pack (x, y, z) into a 64-bit key. Coordinates are offset to keep
            // them non-negative; A* only explores a bounded box around the
            // start, so 21/12/21 bits is plenty of headroom.
            inline uint64_t PackKey(int x, int y, int z)
            {
                const uint64_t ux = static_cast<uint64_t>(x + (1 << 20)) & ((1ULL << 21) - 1);
                const uint64_t uy = static_cast<uint64_t>(y + (1 << 11)) & ((1ULL << 12) - 1);
                const uint64_t uz = static_cast<uint64_t>(z + (1 << 20)) & ((1ULL << 21) - 1);
                return (ux << 33) | (uy << 21) | uz;
            }

            struct NodeInfo
            {
                float    fG;            // accumulated cost (seconds)
                int      iPrevX, iPrevY, iPrevZ;
                bool     bClosed;
                bool     bEnterBreak;   // entering this cell required breaking a wall
            };

            struct OpenEntry
            {
                float fF;        // g + h
                int   x, y, z;
                bool  operator<(const OpenEntry& o) const { return fF > o.fF; }   // min-heap
            };
        }

        bool FindPath(const Engine::VoxelWorld& world,
                      int sx, int sy, int sz,
                      int ex, int ey, int ez,
                      float fSpeed,
                      int iSearchRange,
                      std::vector<PathStep>& outPath)
        {
            outPath.clear();
            if (fSpeed <= 0.f) return false;

            if (Engine::IsSolid(world.GetBlock(sx, sy, sz))) return false;

            const float fMoveCost = 1.f / fSpeed;

            std::priority_queue<OpenEntry> open;
            std::unordered_map<uint64_t, NodeInfo> visited;

            // xz-only Manhattan heuristic — admissible because y-changes
            // carry no extra cost in the transition model.
            const auto heuristic = [&](int x, int z) {
                return (std::abs(ex - x) + std::abs(ez - z)) * fMoveCost;
            };

            visited[PackKey(sx, sy, sz)] = { 0.f, sx, sy, sz, false, false };
            open.push({ heuristic(sx, sz), sx, sy, sz });

            const int dx[4] = { 1, -1, 0,  0 };
            const int dz[4] = { 0,  0, 1, -1 };

            // Cap the vertical fall distance so a single neighbour expansion
            // doesn't scan to infinity when the column has no floor below.
            const int iFallCap = iSearchRange + 2;

            while (!open.empty())
            {
                const OpenEntry cur = open.top(); open.pop();

                auto itCur = visited.find(PackKey(cur.x, cur.y, cur.z));
                if (itCur == visited.end()) continue;
                if (itCur->second.bClosed)  continue;
                itCur->second.bClosed = true;

                // Goal test on xz — y is whatever the search arrived at, since
                // the start/end planes can differ (player on a 1-block ledge).
                if (cur.x == ex && cur.z == ez)
                {
                    std::vector<PathStep> rev;
                    int cx = cur.x, cy = cur.y, cz = cur.z;
                    while (cx != sx || cy != sy || cz != sz)
                    {
                        auto it = visited.find(PackKey(cx, cy, cz));
                        if (it == visited.end()) return false;
                        rev.push_back({ cx, cy, cz, it->second.bEnterBreak });
                        const int px = it->second.iPrevX;
                        const int py = it->second.iPrevY;
                        const int pz = it->second.iPrevZ;
                        cx = px; cy = py; cz = pz;
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
                const int   cy   = cur.y;

                for (int i = 0; i < 4; ++i)
                {
                    const int nx = cur.x + dx[i];
                    const int nz = cur.z + dz[i];

                    // Three transition cases mirror Player::Input's auto-step:
                    //   flat   (ny = cy)     : floor at cy-1 is solid, cell at
                    //                          cy is air (or breakable).
                    //   step-up(ny = cy+1)   : cell at cy is solid, cell at
                    //                          cy+1 is air, AND the cell above
                    //                          the mover (cur.x, cy+1, cur.z)
                    //                          is air (headroom to lift).
                    //   fall   (ny < cy)     : cell at cy is air and floor at
                    //                          cy-1 is air — drop to the first
                    //                          solid below in this column.
                    // Wall-breaking is only considered in the flat case.

                    const Engine::BlockType bFloor = world.GetBlock(nx, cy - 1, nz);
                    const Engine::BlockType bSide  = world.GetBlock(nx, cy,     nz);
                    const bool bFloorSolid = Engine::IsSolid(bFloor);
                    const bool bSideSolid  = Engine::IsSolid(bSide);

                    int   ny         = cy;
                    bool  bBreakHere = false;
                    float fBreakCost = 0.f;
                    bool  bValid     = false;

                    if (bFloorSolid)
                    {
                        if (!bSideSolid)
                        {
                            // Flat walk.
                            ny     = cy;
                            bValid = true;
                        }
                        else
                        {
                            // Try step-up first (free), fall back to break (costs time).
                            const bool bStepAir =
                                !Engine::IsSolid(world.GetBlock(nx, cy + 1, nz));
                            const bool bHeadAir =
                                !Engine::IsSolid(world.GetBlock(cur.x, cy + 1, cur.z));
                            if (bStepAir && bHeadAir)
                            {
                                ny     = cy + 1;
                                bValid = true;
                            }
                            else
                            {
                                const float fBreak = Engine::BlockBreakTime(bSide);
                                if (fBreak >= 0.f)
                                {
                                    ny         = cy;
                                    bBreakHere = true;
                                    fBreakCost = fBreak;
                                    bValid     = true;
                                }
                            }
                        }
                    }
                    else if (!bSideSolid)
                    {
                        // No floor at cy-1 and side is air — fall.
                        int fy = cy - 1;
                        const int fyMin = cy - iFallCap;
                        while (fy > fyMin && !Engine::IsSolid(world.GetBlock(nx, fy, nz)))
                            --fy;
                        if (Engine::IsSolid(world.GetBlock(nx, fy, nz)))
                        {
                            ny     = fy + 1;
                            bValid = true;
                        }
                    }

                    if (!bValid) continue;

                    const float fEnterCost = fMoveCost + (bBreakHere ? fBreakCost : 0.f);
                    const float gNext = gCur + fEnterCost;
                    const uint64_t k = PackKey(nx, ny, nz);
                    auto it = visited.find(k);
                    if (it != visited.end())
                    {
                        if (it->second.bClosed)   continue;
                        if (gNext >= it->second.fG) continue;
                        it->second.fG          = gNext;
                        it->second.iPrevX      = cur.x;
                        it->second.iPrevY      = cur.y;
                        it->second.iPrevZ      = cur.z;
                        it->second.bEnterBreak = bBreakHere;
                    }
                    else
                    {
                        visited[k] = { gNext, cur.x, cur.y, cur.z, false, bBreakHere };
                    }
                    open.push({ gNext + heuristic(nx, nz), nx, ny, nz });
                }
            }

            return false;
        }
    }
}

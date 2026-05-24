#include "FlowField.h"
#include "../GameDefs.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include <cmath>
#include <limits>
#include <queue>

namespace Client
{
    namespace
    {
        // 8 neighbours: 0..3 = +X,-X,+Z,-Z (orthogonal), 4..7 = diagonals.
        // Diagonal entries list their two orthogonal "shoulders" so the
        // corner-cut check can read them straight off the table.
        struct Neighbour
        {
            int  dx, dz;
            float fCost;        // base move cost; break-cost added per-cell
            int  iOrthoA, iOrthoB;   // -1 for orthogonal entries
        };

        constexpr float kSqrt2 = 1.41421356f;

        const Neighbour kNbr[8] = {
            { +1,  0, 1.f,        -1, -1 },
            { -1,  0, 1.f,        -1, -1 },
            {  0, +1, 1.f,        -1, -1 },
            {  0, -1, 1.f,        -1, -1 },
            { +1, +1, kSqrt2,      0,  2 },
            { +1, -1, kSqrt2,      0,  3 },
            { -1, +1, kSqrt2,      1,  2 },
            { -1, -1, kSqrt2,      1,  3 },
        };

        struct OpenEntry
        {
            float fG;
            int   iIdx;
            bool operator<(const OpenEntry& o) const { return fG > o.fG; }   // min-heap
        };
    }

    FlowField::FlowField()
        : m_vG  (FlowField::kSide * FlowField::kSide,
                 std::numeric_limits<float>::infinity()),
          m_vDir(FlowField::kSide * FlowField::kSide, int8_t(-1))
    {
    }

    int FlowField::Index(int cx, int cz) const
    {
        const int lx = cx - m_iOriginX;
        const int lz = cz - m_iOriginZ;
        if (lx < 0 || lx >= kSide || lz < 0 || lz >= kSide) return -1;
        return lz * kSide + lx;
    }

    bool FlowField::Rebuild(const Engine::VoxelWorld& world, int gx, int gz)
    {
        if (m_bHasGoal && gx == m_iGoalX && gz == m_iGoalZ) return false;
        return ForceRebuild(world, gx, gz);
    }

    bool FlowField::ForceRebuild(const Engine::VoxelWorld& world, int gx, int gz)
    {
        m_iGoalX   = gx;
        m_iGoalZ   = gz;
        m_iOriginX = gx - kRadius;
        m_iOriginZ = gz - kRadius;
        m_bHasGoal = true;

        std::fill(m_vG.begin(),   m_vG.end(),   std::numeric_limits<float>::infinity());
        std::fill(m_vDir.begin(), m_vDir.end(), int8_t(-1));

        // Pre-bake the local block grid once per rebuild. The Dijkstra
        // inner loop hits each cell up to 16 times (8 destinations + 8
        // shoulder checks for diagonals); going through VoxelWorld::
        // GetBlock → std::map::find every time turned this into a 27%
        // CPU spike on cell-cross frames. One linear sweep here, then
        // the loop reads from a flat array.
        //
        // 1-cell padding lets the shoulder/destination lookups skip a
        // bounds check; off-grid cells are stamped as Stone so diagonals
        // at the field boundary are blocked (conservative — Dijkstra
        // can't propagate outside the window anyway).
        constexpr int kPad      = 1;
        constexpr int kCacheSide = kSide + kPad * 2;
        std::vector<Engine::BlockType> blocks(
            kCacheSide * kCacheSide, Engine::BlockType::Stone);
        for (int lz = 0; lz < kSide; ++lz)
        {
            const int wz = m_iOriginZ + lz;
            for (int lx = 0; lx < kSide; ++lx)
            {
                const int wx = m_iOriginX + lx;
                blocks[(lz + kPad) * kCacheSide + (lx + kPad)] =
                    world.GetBlock(wx, kWallY, wz);
            }
        }
        const auto BlockAt = [&](int cx, int cz) -> Engine::BlockType
        {
            const int lx = cx - m_iOriginX + kPad;
            const int lz = cz - m_iOriginZ + kPad;
            return blocks[lz * kCacheSide + lx];
        };

        // Goal must be air — if it isn't, no enemy can reach it; leave
        // the field empty and let callers fall back to straight-line.
        if (Engine::IsSolid(BlockAt(gx, gz))) return true;

        const int iGoal = Index(gx, gz);
        m_vG[iGoal] = 0.f;

        std::priority_queue<OpenEntry> open;
        open.push({ 0.f, iGoal });

        while (!open.empty())
        {
            const OpenEntry cur = open.top(); open.pop();
            if (cur.fG > m_vG[cur.iIdx]) continue;   // stale heap entry

            const int lx = cur.iIdx % kSide;
            const int lz = cur.iIdx / kSide;
            const int cx = lx + m_iOriginX;
            const int cz = lz + m_iOriginZ;

            // Probe each neighbour as a *successor* of the goal-expansion.
            // We're running Dijkstra from the goal outward, so the cost to
            // reach (cx, cz) from (nx, nz) is paid when *entering* (cx, cz)
            // — wait, that's backwards for a flow field. Reverse direction
            // semantics: when an enemy at (nx, nz) reads the field, it
            // wants the neighbour that moves it closer to the goal. We
            // compute that by treating each cell's cost as "cost an enemy
            // pays to reach the goal *from* this cell", which equals the
            // cost paid to *enter* this cell from a closer neighbour. So
            // when relaxing from (cx, cz) to (nx, nz) we add the enter-
            // cost of (nx, nz) — solid (nx, nz) tacks on its break time.
            for (int i = 0; i < 8; ++i)
            {
                const Neighbour& n = kNbr[i];
                const int nx = cx + n.dx;
                const int nz = cz + n.dz;

                const int iN = Index(nx, nz);
                if (iN < 0) continue;

                const Engine::BlockType b = BlockAt(nx, nz);
                const bool bSolid = Engine::IsSolid(b);
                float fBreak = 0.f;
                if (bSolid)
                {
                    fBreak = Engine::BlockBreakTime(b);
                    if (fBreak < 0.f) continue;   // unbreakable
                }

                // Corner-cut guard for diagonals — both shoulder cells
                // must be air. Even a breakable shoulder blocks the
                // diagonal because an enemy gliding across would
                // visually clip the wall corner; let the route go
                // through the orthogonals and break the wall there.
                if (n.iOrthoA >= 0)
                {
                    const Neighbour& a = kNbr[n.iOrthoA];
                    const Neighbour& c = kNbr[n.iOrthoB];
                    if (Engine::IsSolid(BlockAt(cx + a.dx, cz + a.dz))) continue;
                    if (Engine::IsSolid(BlockAt(cx + c.dx, cz + c.dz))) continue;
                }

                const float fNewG = cur.fG + n.fCost + fBreak;
                if (fNewG >= m_vG[iN]) continue;

                m_vG[iN]   = fNewG;
                // Direction recorded on (nx, nz) points back toward (cx, cz)
                // — that's "go this way to reach the goal." Encode as the
                // neighbour index that an enemy at (nx, nz) would take.
                // (cx, cz) - (nx, nz) = -(n.dx, n.dz), so the reverse
                // neighbour index is the one with (-dx, -dz). Pairs are
                // arranged so reverse-of-i is i^1 for both ortho (0..3)
                // and diagonals (4..7): 0<->1, 2<->3, 4<->7, 5<->6 — the
                // last two aren't xor pairs, so look it up.
                int iRev = -1;
                switch (i)
                {
                    case 0: iRev = 1; break;
                    case 1: iRev = 0; break;
                    case 2: iRev = 3; break;
                    case 3: iRev = 2; break;
                    case 4: iRev = 7; break;
                    case 5: iRev = 6; break;
                    case 6: iRev = 5; break;
                    case 7: iRev = 4; break;
                }
                m_vDir[iN] = static_cast<int8_t>(iRev);
                open.push({ fNewG, iN });
            }
        }
        return true;
    }

    bool FlowField::Sample(int cx, int cz,
                           int& outNextX, int& outNextZ,
                           Engine::Vector3& outDir) const
    {
        const int iIdx = Index(cx, cz);
        if (iIdx < 0) return false;
        const int8_t d = m_vDir[iIdx];
        if (d < 0) return false;

        const Neighbour& n = kNbr[d];
        outNextX = cx + n.dx;
        outNextZ = cz + n.dz;

        // Normalised xz direction toward the next cell centre. y=0 keeps
        // the enemy on the wall layer.
        const float fInv = 1.f / n.fCost;   // n.fCost is the vector length
        outDir = Engine::Vector3(n.dx * fInv, 0.f, n.dz * fInv);
        return true;
    }
}

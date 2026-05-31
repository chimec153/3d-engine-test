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

    uint64_t FlowField::HashBlocked(const std::vector<std::pair<int, int>>& v)
    {
        // XOR of per-cell mixes — order-independent so the caller doesn't have
        // to sort before each call. Splitmix-style mix on a packed 64-bit cell
        // key keeps the distribution clean enough for short lists (~tens of
        // towers), and the size is XOR'd in so an empty list isn't 0.
        uint64_t h = static_cast<uint64_t>(v.size()) * 0x9E3779B97F4A7C15ULL;
        for (const auto& p : v)
        {
            uint64_t k = (static_cast<uint64_t>(static_cast<uint32_t>(p.first)) << 32)
                       |  static_cast<uint64_t>(static_cast<uint32_t>(p.second));
            k ^= k >> 30; k *= 0xBF58476D1CE4E5B9ULL;
            k ^= k >> 27; k *= 0x94D049BB133111EBULL;
            k ^= k >> 31;
            h ^= k;
        }
        return h;
    }

    bool FlowField::Rebuild(const Engine::VoxelWorld& world, int gx, int gz,
                            const std::vector<std::pair<int, int>>& vecBlocked)
    {
        const uint64_t uFp = HashBlocked(vecBlocked);
        if (m_bHasGoal && gx == m_iGoalX && gz == m_iGoalZ &&
            uFp == m_uBlockedFingerprint)
            return false;
        return ForceRebuild(world, gx, gz, vecBlocked);
    }

    bool FlowField::ForceRebuild(const Engine::VoxelWorld& world, int gx, int gz,
                                 const std::vector<std::pair<int, int>>& vecBlocked)
    {
        m_iGoalX   = gx;
        m_iGoalZ   = gz;
        m_iOriginX = gx - kRadius;
        m_iOriginZ = gz - kRadius;
        m_bHasGoal = true;
        m_uBlockedFingerprint = HashBlocked(vecBlocked);
        m_vBlocked = vecBlocked;

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
        // Parallel "tower-occupied" overlay. Same layout as `blocks` (with the
        // 1-cell padding) so the shoulder/destination lookups can read both
        // arrays without an extra bounds check. Pad cells default to false —
        // they're already Stone in `blocks`, so they're unbreakable regardless.
        std::vector<uint8_t> blocked(kCacheSide * kCacheSide, uint8_t(0));
        for (const auto& p : vecBlocked)
        {
            const int lx = p.first  - m_iOriginX + kPad;
            const int lz = p.second - m_iOriginZ + kPad;
            if (lx < kPad || lx >= kCacheSide - kPad) continue;
            if (lz < kPad || lz >= kCacheSide - kPad) continue;
            blocked[lz * kCacheSide + lx] = 1;
        }
        const auto BlockAt = [&](int cx, int cz) -> Engine::BlockType
        {
            const int lx = cx - m_iOriginX + kPad;
            const int lz = cz - m_iOriginZ + kPad;
            return blocks[lz * kCacheSide + lx];
        };
        const auto IsTowerBlocked = [&](int cx, int cz) -> bool
        {
            const int lx = cx - m_iOriginX + kPad;
            const int lz = cz - m_iOriginZ + kPad;
            return blocked[lz * kCacheSide + lx] != 0;
        };

        // Goal cell: only bail when the *voxel* is solid. A tower-occupied
        // goal (enemies chasing a tower aggro target) is fine — Dijkstra
        // plants cost 0 here and expands outward without ever needing to
        // "enter" the goal cell, so neighbours can record a direction toward
        // it and adjacent enemies still get a melee approach.
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

                // Tower-occupied cells are unbreakable to pathing — skip
                // outright (no break-cost branch). Visually the voxel is
                // still Air, but logically it's a permanent wall until the
                // tower is destroyed (which triggers a rebuild via the
                // spawner's blocked-set fingerprint).
                if (IsTowerBlocked(nx, nz)) continue;

                const Engine::BlockType b = BlockAt(nx, nz);
                const bool bSolid = Engine::IsSolid(b);
                float fBreak = 0.f;
                if (bSolid)
                {
                    fBreak = Engine::BlockBreakTime(b);
                    if (fBreak < 0.f) continue;   // unbreakable
                }

                // Corner-cut guard for diagonals — both shoulder cells
                // must be air AND not tower-occupied. A breakable wall or
                // a tower on the shoulder blocks the diagonal so the route
                // is forced through an orthogonal step (which then either
                // breaks the wall or routes around the tower).
                if (n.iOrthoA >= 0)
                {
                    const Neighbour& a = kNbr[n.iOrthoA];
                    const Neighbour& c = kNbr[n.iOrthoB];
                    const int saX = cx + a.dx, saZ = cz + a.dz;
                    const int scX = cx + c.dx, scZ = cz + c.dz;
                    if (Engine::IsSolid(BlockAt(saX, saZ)) || IsTowerBlocked(saX, saZ)) continue;
                    if (Engine::IsSolid(BlockAt(scX, scZ)) || IsTowerBlocked(scX, scZ)) continue;
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

    bool FlowField::IsBlocked(int cx, int cz) const
    {
        for (const auto& p : m_vBlocked)
            if (p.first == cx && p.second == cz) return true;
        return false;
    }

    bool FlowField::Reaches(int cx, int cz) const
    {
        if (!m_bHasGoal) return false;
        const int iIdx = Index(cx, cz);
        if (iIdx < 0) return false;
        // Unreached cells keep the +inf sentinel from the rebuild fill; the
        // goal itself is 0 and every expanded cell is finite.
        return m_vG[iIdx] < std::numeric_limits<float>::infinity();
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

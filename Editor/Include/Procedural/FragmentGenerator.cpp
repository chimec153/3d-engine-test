#include "FragmentGenerator.h"

#include "Types.h"
#include "Vector3.h"
#include "Bindable/Mesh.h"

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

// NOTE: <random> is deliberately NOT used. Engine/Include/Core/Macro.h defines
// a global `#define epsilon 0.0001`, and <random> pulls the 128-bit
// numeric_limits specialization (__msvc_int128.hpp) whose epsilon() member then
// gets macro-mangled into a syntax error. The inline xorshift PRNG below avoids
// the whole dependency.

namespace Editor
{
    // Unity (jumbo) build: file-local helpers MUST live in a uniquely named
    // namespace, not an anonymous one, or they collide with same-named helpers
    // from sibling .cpp files merged into the same translation unit.
    namespace fraggen_detail
    {
        using V3 = Engine::Vector3;

        struct Tri { V3 v[3]; };
        using Soup = std::vector<Tri>;

        constexpr float kEps = 1e-5f;

        // Deterministic xorshift32 — dependency-free, plenty for cut jitter.
        struct Rng
        {
            unsigned int s;
            explicit Rng(unsigned int seed) : s(seed ? seed : 0x9E3779B9u) {}
            unsigned int Next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
            float Unit() { return (Next() >> 8) * (1.f / 16777216.f); }   // [0,1)
            float Range(float a, float b) { return a + (b - a) * Unit(); }
        };

        inline V3 Normalized(const V3& v)
        {
            float len = v.Length();
            if (len < 1e-8f) return V3(0.f, 0.f, 0.f);
            return v / len;
        }

        // Signed distance of p from plane { x : dot(n,x) = d }.
        inline float Dist(const V3& n, float d, const V3& p) { return n.Dot(p) - d; }

        // ---- base volumes (built with outward-facing winding) ----------------

        // Force every base triangle to wind so cross(v1-v0, v2-v0) points away
        // from the origin. The base shapes are origin-centred and convex, so
        // "outward" is simply the direction from the origin to the face. This
        // is the engine's front-face convention (see MeshPresets::AxisBox).
        void FixOutward(Soup& s)
        {
            for (auto& t : s)
            {
                V3 nrm = (t.v[1] - t.v[0]).Cross(t.v[2] - t.v[0]);
                V3 mid = (t.v[0] + t.v[1] + t.v[2]);
                if (nrm.Dot(mid) < 0.f) std::swap(t.v[1], t.v[2]);
            }
        }

        // h is the X/Z half-extent (footprint); hy is the Y half-extent
        // (height). Equal values give a cube (the original behaviour).
        Soup MakeBox(float h, float hy)
        {
            const V3 c[8] = {
                {-h,-hy,-h}, { h,-hy,-h}, { h, hy,-h}, {-h, hy,-h},
                {-h,-hy, h}, { h,-hy, h}, { h, hy, h}, {-h, hy, h},
            };
            // 6 quads -> 12 tris. FixOutward corrects the winding afterwards,
            // so the corner order here only has to enclose each face.
            const int q[6][4] = {
                {0,1,2,3}, {5,4,7,6}, {4,5,1,0},
                {3,2,6,7}, {1,5,6,2}, {4,0,3,7},
            };
            Soup s;
            for (auto& f : q)
            {
                s.push_back({{ c[f[0]], c[f[1]], c[f[2]] }});
                s.push_back({{ c[f[0]], c[f[2]], c[f[3]] }});
            }
            FixOutward(s);
            return s;
        }

        // Capsule centred at origin, cylinder axis along +Y. fCylHeight is the
        // straight section; the two hemisphere caps add r each, so total height
        // is fCylHeight + 2*r. Convex by construction → safe for the same
        // plane-clip pipeline that handles Box/Sphere.
        Soup MakeCapsule(float r, float fCylHeight, int iRings, int iSectors)
        {
            iRings   = std::max(2, iRings);
            iSectors = std::max(4, iSectors);
            const float fHalf = std::max(0.f, fCylHeight) * 0.5f;
            const float kPI   = 3.14159265358979323846f;

            // Build a ring of vertices for each latitude band, then triangulate
            // between successive rings. Bands include the top/bottom poles as
            // degenerate (single-point) rings so the quad-strip logic stays
            // uniform.
            //
            // Layout (latitude index 0 .. 2*iRings):
            //   0                 → +Y pole
            //   1 .. iRings-1     → upper hemisphere bands (y >= +fHalf)
            //   iRings .. iRings+1→ cylinder top (+fHalf) and bottom (-fHalf)
            //   ...               → lower hemisphere bands
            //   2*iRings          → -Y pole
            const int kBands = 2 * iRings;
            std::vector<std::vector<V3>> rings(kBands + 1);

            for (int i = 0; i <= kBands; ++i)
            {
                float t;
                V3    centre;
                float radius;

                if (i <= iRings)
                {
                    // Upper hemisphere: t goes 0..1 from pole down to the
                    // equator that meets the cylinder top.
                    t          = (float)i / (float)iRings;
                    float phi  = (kPI * 0.5f) * (1.f - t);     // pi/2 → 0
                    centre     = V3(0.f, fHalf + r * std::sin(phi), 0.f);
                    radius     = r * std::cos(phi);
                }
                else
                {
                    // Lower hemisphere: t goes 0..1 from cylinder bottom down
                    // to the pole.
                    int   j   = i - iRings;
                    float u   = (float)j / (float)iRings;
                    float phi = (kPI * 0.5f) * u;              // 0 → pi/2
                    centre    = V3(0.f, -fHalf - r * std::sin(phi), 0.f);
                    radius    = r * std::cos(phi);
                }

                if (radius < kEps)
                {
                    rings[i].push_back(centre);
                }
                else
                {
                    rings[i].reserve(iSectors);
                    for (int s = 0; s < iSectors; ++s)
                    {
                        float theta = (2.f * kPI) * (float)s / (float)iSectors;
                        rings[i].push_back(V3(
                            centre.x + radius * std::cos(theta),
                            centre.y,
                            centre.z + radius * std::sin(theta)));
                    }
                }
            }

            Soup s;
            s.reserve(kBands * iSectors * 2);
            for (int i = 0; i < kBands; ++i)
            {
                const auto& a = rings[i];
                const auto& b = rings[i + 1];
                const bool aPole = (a.size() == 1);
                const bool bPole = (b.size() == 1);

                for (int q = 0; q < iSectors; ++q)
                {
                    int q1 = (q + 1) % iSectors;
                    V3 a0 = aPole ? a[0] : a[q];
                    V3 a1 = aPole ? a[0] : a[q1];
                    V3 b0 = bPole ? b[0] : b[q];
                    V3 b1 = bPole ? b[0] : b[q1];

                    if (aPole)
                    {
                        s.push_back({{ a0, b0, b1 }});
                    }
                    else if (bPole)
                    {
                        s.push_back({{ a0, a1, b0 }});
                    }
                    else
                    {
                        s.push_back({{ a0, a1, b1 }});
                        s.push_back({{ a0, b1, b0 }});
                    }
                }
            }
            FixOutward(s);
            return s;
        }

        // Cylinder centred at origin, axis along +Y. Radius r, half-height hy
        // (total height = 2*hy), iSectors longitude segments, flat caps. Convex
        // by construction → safe for the same plane-clip pipeline. FixOutward
        // fixes the winding, so the triangle vertex order below only has to
        // enclose each face.
        Soup MakeCylinder(float r, float hy, int iSectors)
        {
            iSectors = std::max(3, iSectors);
            const float kPI = 3.14159265358979323846f;

            std::vector<V3> top, bot;
            top.reserve(iSectors);
            bot.reserve(iSectors);
            for (int s = 0; s < iSectors; ++s)
            {
                float theta = (2.f * kPI) * (float)s / (float)iSectors;
                float cx = r * std::cos(theta);
                float cz = r * std::sin(theta);
                top.push_back(V3(cx,  hy, cz));
                bot.push_back(V3(cx, -hy, cz));
            }
            const V3 cTop(0.f,  hy, 0.f);
            const V3 cBot(0.f, -hy, 0.f);

            Soup s;
            s.reserve(iSectors * 4);
            for (int q = 0; q < iSectors; ++q)
            {
                int q1 = (q + 1) % iSectors;
                // Side wall (two tris per sector).
                s.push_back({{ top[q], top[q1], bot[q1] }});
                s.push_back({{ top[q], bot[q1], bot[q]  }});
                // Top + bottom cap fans.
                s.push_back({{ cTop, top[q], top[q1] }});
                s.push_back({{ cBot, bot[q], bot[q1] }});
            }
            FixOutward(s);
            return s;
        }

        Soup MakeIcosphere(float r, int subdiv)
        {
            const float t = (1.f + std::sqrt(5.f)) * 0.5f;
            std::vector<V3> v = {
                {-1, t, 0}, { 1, t, 0}, {-1,-t, 0}, { 1,-t, 0},
                { 0,-1, t}, { 0, 1, t}, { 0,-1,-t}, { 0, 1,-t},
                { t, 0,-1}, { t, 0, 1}, {-t, 0,-1}, {-t, 0, 1},
            };
            std::vector<std::array<int,3>> f = {
                {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
                {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
                {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
                {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1},
            };

            subdiv = std::max(0, std::min(subdiv, 4));
            for (int it = 0; it < subdiv; ++it)
            {
                std::vector<std::array<int,3>> nf;
                nf.reserve(f.size() * 4);
                std::vector<long long> cache;          // (parallel) edge keys
                std::vector<int>       cacheIdx;
                auto midpoint = [&](int a, int b) -> int
                {
                    long long key = (long long)std::min(a,b) * 100000ll + std::max(a,b);
                    for (size_t i = 0; i < cache.size(); ++i)
                        if (cache[i] == key) return cacheIdx[i];
                    V3 m = (v[a] + v[b]) * 0.5f;
                    int idx = (int)v.size();
                    v.push_back(m);
                    cache.push_back(key);
                    cacheIdx.push_back(idx);
                    return idx;
                };
                for (auto& tri : f)
                {
                    int a = midpoint(tri[0], tri[1]);
                    int b = midpoint(tri[1], tri[2]);
                    int c = midpoint(tri[2], tri[0]);
                    nf.push_back({tri[0], a, c});
                    nf.push_back({tri[1], b, a});
                    nf.push_back({tri[2], c, b});
                    nf.push_back({a, b, c});
                }
                f.swap(nf);
            }

            // Project every vertex onto the sphere of radius r.
            for (auto& p : v) p = Normalized(p) * r;

            Soup s;
            s.reserve(f.size());
            for (auto& tri : f) s.push_back({{ v[tri[0]], v[tri[1]], v[tri[2]] }});
            FixOutward(s);
            return s;
        }

        // ---- plane clipping --------------------------------------------------

        // Clip a polygon against one half-space (Sutherland-Hodgman). keepPos:
        // keep the dot(n,x) >= d side, else the <= d side. Records new edge-plane
        // intersection points into `cut` (used to build the cap polygon).
        void ClipHalf(const std::vector<V3>& poly, const V3& n, float d, bool keepPos,
                      std::vector<V3>& out, std::vector<V3>* cut)
        {
            out.clear();
            const size_t cnt = poly.size();
            for (size_t i = 0; i < cnt; ++i)
            {
                const V3& A = poly[i];
                const V3& B = poly[(i + 1) % cnt];
                const float da = Dist(n, d, A);
                const float db = Dist(n, d, B);
                const bool inA = keepPos ? (da >= -kEps) : (da <= kEps);

                if (inA) out.push_back(A);

                // Strict crossing only — grazing vertices are handled by inA.
                if ((da > kEps && db < -kEps) || (da < -kEps && db > kEps))
                {
                    const float s = da / (da - db);
                    V3 P = A + (B - A) * s;
                    out.push_back(P);
                    if (cut) cut->push_back(P);
                }
            }
        }

        void AddFan(Soup& s, const std::vector<V3>& poly)
        {
            if (poly.size() < 3) return;
            for (size_t i = 1; i + 1 < poly.size(); ++i)
                s.push_back({{ poly[0], poly[i], poly[i + 1] }});
        }

        // Dedup the raw cut points and order them into a convex loop, CCW about
        // +n. For a convex solid the cross-section is one convex polygon, so an
        // angular sort around the centroid recovers the boundary exactly.
        std::vector<V3> OrderCrossSection(const std::vector<V3>& raw, const V3& n)
        {
            std::vector<V3> pts;
            for (const V3& p : raw)
            {
                bool dup = false;
                for (const V3& q : pts)
                    if (p.DistanceSq(q) < kEps * kEps) { dup = true; break; }
                if (!dup) pts.push_back(p);
            }
            if (pts.size() < 3) return {};

            V3 c(0.f, 0.f, 0.f);
            for (const V3& p : pts) c = c + p;
            c = c / (float)pts.size();

            // Build an in-plane basis (u, w) with w = n x u.
            V3 ref = (std::fabs(n.x) < 0.9f) ? V3(1.f, 0.f, 0.f) : V3(0.f, 1.f, 0.f);
            V3 u = Normalized(n.Cross(ref));
            V3 w = n.Cross(u);

            std::sort(pts.begin(), pts.end(), [&](const V3& a, const V3& b)
            {
                float aa = std::atan2((a - c).Dot(w), (a - c).Dot(u));
                float ab = std::atan2((b - c).Dot(w), (b - c).Dot(u));
                return aa < ab;
            });
            return pts;
        }

        // Cap the hole with a centroid fan. `reverse` flips the winding so the
        // computed flat normal points outward for the shard being capped: the
        // back shard (on the -n side) wants +n, the front shard wants -n.
        void AddCap(Soup& s, const std::vector<V3>& loop, bool reverse)
        {
            const size_t k = loop.size();
            if (k < 3) return;
            V3 c(0.f, 0.f, 0.f);
            for (const V3& p : loop) c = c + p;
            c = c / (float)k;
            for (size_t i = 0; i < k; ++i)
            {
                const V3& a = loop[i];
                const V3& b = loop[(i + 1) % k];
                if (!reverse) s.push_back({{ c, a, b }});
                else          s.push_back({{ c, b, a }});
            }
        }

        void Split(const Soup& in, const V3& n, float d, Soup& front, Soup& back)
        {
            std::vector<V3> cut, fp, bp;
            for (const Tri& t : in)
            {
                std::vector<V3> poly = { t.v[0], t.v[1], t.v[2] };
                ClipHalf(poly, n, d, true,  fp, &cut);
                ClipHalf(poly, n, d, false, bp, nullptr);
                AddFan(front, fp);
                AddFan(back,  bp);
            }
            std::vector<V3> loop = OrderCrossSection(cut, n);
            if (loop.size() >= 3)
            {
                AddCap(back,  loop, false);  // back shard: outward normal +n
                AddCap(front, loop, true);   // front shard: outward normal -n
            }
        }

        float Volume(const Soup& s)
        {
            float v = 0.f;
            for (const Tri& t : s) v += t.v[0].Dot(t.v[1].Cross(t.v[2]));
            return std::fabs(v) / 6.f;
        }

        void Bounds(const Soup& s, V3& lo, V3& hi)
        {
            lo = V3( 1e9f,  1e9f,  1e9f);
            hi = V3(-1e9f, -1e9f, -1e9f);
            for (const Tri& t : s)
                for (const V3& p : t.v)
                {
                    lo.x = std::min(lo.x, p.x); hi.x = std::max(hi.x, p.x);
                    lo.y = std::min(lo.y, p.y); hi.y = std::max(hi.y, p.y);
                    lo.z = std::min(lo.z, p.z); hi.z = std::max(hi.z, p.z);
                }
        }

        // Build a flat-shaded VertexStandard container from a shard. Degenerate
        // (zero-area) triangles are dropped so no NaN normals slip through.
        void BuildContainer(const Soup& s,
                            std::vector<Engine::VertexStandard>& verts,
                            std::vector<unsigned int>& inds)
        {
            verts.clear(); inds.clear();
            const DirectX::XMFLOAT2 uv[3] = { {0.f,0.f}, {1.f,0.f}, {0.f,1.f} };
            for (const Tri& t : s)
            {
                V3 raw = (t.v[1] - t.v[0]).Cross(t.v[2] - t.v[0]);
                if (raw.Length() < 1e-7f) continue;     // skip slivers
                V3 nrm = Normalized(raw);
                const unsigned int base = (unsigned int)verts.size();
                for (int k = 0; k < 3; ++k)
                {
                    Engine::VertexStandard v;
                    v.pos    = t.v[k];
                    v.normal = nrm;
                    v.uv     = uv[k];
                    v.tangent.x = 1.f; v.tangent.w = 1.f;
                    verts.push_back(v);
                    inds.push_back(base + k);
                }
            }
        }
    }

    FragmentGenerator::Result FragmentGenerator::Generate(const Params& params)
    {
        using namespace fraggen_detail;

        const float size = std::max(0.01f, params.fSize);
        Soup base;
        switch (params.eShape)
        {
        case BaseShape::Sphere:
            base = MakeIcosphere(size, params.iSphereSubdiv);
            break;
        case BaseShape::Capsule:
            base = MakeCapsule(size, params.fCylHeight,
                               params.iCapsuleRings, params.iCapsuleSectors);
            break;
        case BaseShape::Cylinder:
            base = MakeCylinder(size, std::max(0.01f, params.fCylinderHeight) * 0.5f,
                                params.iCapsuleSectors);
            break;
        case BaseShape::Box:
        default:
            base = MakeBox(size, std::max(0.01f, params.fBoxHeight));
            break;
        }

        std::vector<Soup> shards;
        shards.push_back(std::move(base));

        const int target = std::max(1, std::min(params.iFragments, 256));

        Rng rng(params.uSeed);
        auto frand = [&](float a, float b) { return rng.Range(a, b); };
        auto randUnit = [&]() -> V3
        {
            float z = frand(-1.f, 1.f);
            float a = frand(0.f, 6.2831853f);
            float r = std::sqrt(std::max(0.f, 1.f - z * z));
            return V3(r * std::cos(a), r * std::sin(a), z);
        };

        // Repeatedly split the largest shard until we reach the target count.
        // The guard caps wasted attempts when a near-tangent plane fails to
        // produce two solid halves.
        int guard = 0;
        while ((int)shards.size() < target && guard < target * 20)
        {
            ++guard;

            int bi = 0; float bv = -1.f;
            for (int i = 0; i < (int)shards.size(); ++i)
            {
                float v = Volume(shards[i]);
                if (v > bv) { bv = v; bi = i; }
            }

            V3 lo, hi; Bounds(shards[bi], lo, hi);
            V3 c = (lo + hi) * 0.5f;
            V3 ext = (hi - lo) * 0.5f;
            // Jitter the cut point off-centre so shards vary in size.
            V3 p = c + V3(frand(-0.3f, 0.3f) * ext.x,
                          frand(-0.3f, 0.3f) * ext.y,
                          frand(-0.3f, 0.3f) * ext.z);
            V3 n = randUnit();
            float d = n.Dot(p);

            Soup f, b;
            Split(shards[bi], n, d, f, b);
            if ((int)f.size() >= 4 && (int)b.size() >= 4)
            {
                shards[bi] = std::move(f);
                shards.push_back(std::move(b));
            }
        }

        Result res;
        for (const Soup& shard : shards)
        {
            std::vector<Engine::VertexStandard> verts;
            std::vector<unsigned int>           inds;
            BuildContainer(shard, verts, inds);
            if (verts.empty()) continue;

            if (!res.pMesh) res.pMesh = std::make_shared<Engine::Mesh>(verts, inds);
            else            res.pMesh->CreateMesh(verts, inds);

            ++res.iFragments;
            res.iVertices  += (int)verts.size();
            res.iTriangles += (int)inds.size() / 3;
        }

        if (res.pMesh) res.pMesh->SetTag("__FragmentBake");
        return res;
    }
}

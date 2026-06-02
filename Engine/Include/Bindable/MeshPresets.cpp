#include "MeshPresets.h"
#include "Mesh.h"
#include "Sphere.h"
#include "Box.h"
#include "Capsule.h"
#include "BindableManager.h"
#include <cstdio>
#include <cmath>
#include <vector>

namespace Engine
{
    namespace MeshPresets
    {
        std::shared_ptr<Mesh> UnitSphere(int iRings, int iSectors)
        {
            char szTag[64];
            std::snprintf(szTag, sizeof(szTag),
                "MeshPreset.Sphere.%d.%d", iRings, iSectors);
            if (auto pCached = StaticFindBindable<Mesh>(szTag)) return pCached;

            std::vector<VertexStandard> verts;
            std::vector<unsigned int>   inds;
            Sphere::BuildMesh<VertexStandard>(iRings, iSectors, verts, inds);
            return StaticCreateBindable<Mesh>(szTag, verts, inds);
        }

        std::shared_ptr<Mesh> UnitBox()
        {
            // BindableManager<Mesh>'s ctor pre-creates "Box" at engine
            // startup. Reuse that — no extra mesh instance needed.
            return StaticFindBindable<Mesh>("Box");
        }

        std::shared_ptr<Mesh> UnitTriangle()
        {
            const char* pTag = "MeshPreset.Triangle";
            if (auto pCached = StaticFindBindable<Mesh>(pTag)) return pCached;

            // Bullet sits at RX=-π/2 + RY=yaw, so local +Y → world forward
            // and local +Z → world up. We want the triangle's front face
            // to be the world-up side so a top-down camera can see it;
            // that means the front face must be local +Z. With our
            // CW-front convention, two faces are emitted (indices
            // {0,1,2} and {0,2,1}) so the projectile reads from either
            // side regardless of camera angle.
            std::vector<VertexStandard> verts(3);
            verts[0].pos = { 0.0f,  0.5f, 0.0f };
            verts[1].pos = { 0.5f, -0.5f, 0.0f };
            verts[2].pos = {-0.5f, -0.5f, 0.0f };
            verts[0].normal = verts[1].normal = verts[2].normal = { 0.f, 0.f, 1.f };
            std::vector<unsigned int> inds = { 0, 2, 1,   0, 1, 2 };
            return StaticCreateBindable<Mesh>(pTag, verts, inds);
        }

        std::shared_ptr<Mesh> UnitCapsule(int iRings, int iSectors,
                                          float fCylHeight, float fScale, float fYLift)
        {
            char szTag[96];
            std::snprintf(szTag, sizeof(szTag),
                "MeshPreset.Capsule.%d.%d.%g.%g.%g",
                iRings, iSectors, fCylHeight, fScale, fYLift);
            if (auto pCached = StaticFindBindable<Mesh>(szTag)) return pCached;

            std::vector<VertexStandard> verts;
            std::vector<unsigned int>   inds;
            Capsule::BuildMesh<VertexStandard>(iRings, iSectors, fCylHeight, verts, inds);

            if (fScale != 1.f || fYLift != 0.f)
            {
                for (auto& v : verts)
                {
                    v.pos.x *= fScale;
                    v.pos.y = v.pos.y * fScale + fYLift;
                    v.pos.z *= fScale;
                }
            }
            return StaticCreateBindable<Mesh>(szTag, verts, inds);
        }

        std::shared_ptr<Mesh> AxisBox(const Vector3& vLo, const Vector3& vHi)
        {
            char szTag[128];
            std::snprintf(szTag, sizeof(szTag),
                "MeshPreset.AxisBox.%g_%g_%g.%g_%g_%g",
                vLo.x, vLo.y, vLo.z, vHi.x, vHi.y, vHi.z);
            if (auto pCached = StaticFindBindable<Mesh>(szTag)) return pCached;

            struct Face { Vector3 n; Vector3 v[4]; };
            const Face faces[6] = {
                { { 1.f, 0.f, 0.f}, {{vHi.x,vHi.y,vLo.z},{vHi.x,vHi.y,vHi.z},{vHi.x,vLo.y,vHi.z},{vHi.x,vLo.y,vLo.z}} },
                { {-1.f, 0.f, 0.f}, {{vLo.x,vHi.y,vHi.z},{vLo.x,vHi.y,vLo.z},{vLo.x,vLo.y,vLo.z},{vLo.x,vLo.y,vHi.z}} },
                { { 0.f, 1.f, 0.f}, {{vLo.x,vHi.y,vHi.z},{vHi.x,vHi.y,vHi.z},{vHi.x,vHi.y,vLo.z},{vLo.x,vHi.y,vLo.z}} },
                { { 0.f,-1.f, 0.f}, {{vHi.x,vLo.y,vHi.z},{vLo.x,vLo.y,vHi.z},{vLo.x,vLo.y,vLo.z},{vHi.x,vLo.y,vLo.z}} },
                { { 0.f, 0.f, 1.f}, {{vHi.x,vHi.y,vHi.z},{vLo.x,vHi.y,vHi.z},{vLo.x,vLo.y,vHi.z},{vHi.x,vLo.y,vHi.z}} },
                { { 0.f, 0.f,-1.f}, {{vLo.x,vHi.y,vLo.z},{vHi.x,vHi.y,vLo.z},{vHi.x,vLo.y,vLo.z},{vLo.x,vLo.y,vLo.z}} },
            };
            const DirectX::XMFLOAT2 uv[4] = { {0.f,0.f},{1.f,0.f},{1.f,1.f},{0.f,1.f} };

            std::vector<VertexStandard> verts; verts.reserve(24);
            std::vector<unsigned int>   inds;  inds.reserve(36);

            for (int f = 0; f < 6; ++f)
            {
                const unsigned int base = static_cast<unsigned int>(verts.size());
                for (int v = 0; v < 4; ++v)
                {
                    VertexStandard vs = {};
                    vs.pos    = faces[f].v[v];
                    vs.normal = faces[f].n;
                    vs.uv     = uv[v];
                    verts.push_back(vs);
                }
                inds.push_back(base + 0); inds.push_back(base + 1); inds.push_back(base + 2);
                inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 3);
            }

            return StaticCreateBindable<Mesh>(szTag, verts, inds);
        }

        std::shared_ptr<Mesh> RegularPrism(int iSides, float fRadius,
                                           float fYLo, float fYHi)
        {
            if (iSides < 3)  iSides = 3;
            if (iSides > 32) iSides = 32;
            char szTag[96];
            std::snprintf(szTag, sizeof(szTag),
                "MeshPreset.Prism.%d.%g.%g.%g", iSides, fRadius, fYLo, fYHi);
            if (auto pCached = StaticFindBindable<Mesh>(szTag)) return pCached;

            const float kTwoPi = 6.2831853071795864f;
            std::vector<VertexStandard> verts;
            std::vector<unsigned int>   inds;

            // Side faces — one flat-shaded quad per edge, normal pointing out
            // along the mid-angle radial. Vertex order [topA, topB, botB, botA]
            // with tris (0,1,2),(0,2,3) gives an outward face (CW-front).
            for (int i = 0; i < iSides; ++i)
            {
                const float a0 = kTwoPi * i / iSides;
                const float a1 = kTwoPi * (i + 1) / iSides;
                const float am = (a0 + a1) * 0.5f;
                const Vector3 n = { cosf(am), 0.f, sinf(am) };
                const float c0 = cosf(a0), s0 = sinf(a0);
                const float c1 = cosf(a1), s1 = sinf(a1);
                const unsigned int base = static_cast<unsigned int>(verts.size());
                VertexStandard q[4] = {};
                q[0].pos = { fRadius * c0, fYHi, fRadius * s0 };
                q[1].pos = { fRadius * c1, fYHi, fRadius * s1 };
                q[2].pos = { fRadius * c1, fYLo, fRadius * s1 };
                q[3].pos = { fRadius * c0, fYLo, fRadius * s0 };
                q[0].uv = { 0.f, 0.f }; q[1].uv = { 1.f, 0.f };
                q[2].uv = { 1.f, 1.f }; q[3].uv = { 0.f, 1.f };
                for (int k = 0; k < 4; ++k) { q[k].normal = n; verts.push_back(q[k]); }
                inds.push_back(base + 0); inds.push_back(base + 1); inds.push_back(base + 2);
                inds.push_back(base + 0); inds.push_back(base + 2); inds.push_back(base + 3);
            }

            // Top cap (+Y): centre fan, tris (centre, next, cur) face up.
            {
                const unsigned int c = static_cast<unsigned int>(verts.size());
                VertexStandard vc = {}; vc.pos = { 0.f, fYHi, 0.f }; vc.normal = { 0.f, 1.f, 0.f }; vc.uv = { 0.5f, 0.5f };
                verts.push_back(vc);
                const unsigned int ring = static_cast<unsigned int>(verts.size());
                for (int i = 0; i < iSides; ++i)
                {
                    const float a = kTwoPi * i / iSides;
                    VertexStandard v = {}; v.pos = { fRadius * cosf(a), fYHi, fRadius * sinf(a) };
                    v.normal = { 0.f, 1.f, 0.f }; v.uv = { 0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a) };
                    verts.push_back(v);
                }
                for (int i = 0; i < iSides; ++i)
                {
                    const unsigned int cur = ring + i;
                    const unsigned int nxt = ring + ((i + 1) % iSides);
                    inds.push_back(c); inds.push_back(nxt); inds.push_back(cur);
                }
            }

            // Bottom cap (-Y): centre fan, tris (centre, cur, next) face down.
            {
                const unsigned int c = static_cast<unsigned int>(verts.size());
                VertexStandard vc = {}; vc.pos = { 0.f, fYLo, 0.f }; vc.normal = { 0.f, -1.f, 0.f }; vc.uv = { 0.5f, 0.5f };
                verts.push_back(vc);
                const unsigned int ring = static_cast<unsigned int>(verts.size());
                for (int i = 0; i < iSides; ++i)
                {
                    const float a = kTwoPi * i / iSides;
                    VertexStandard v = {}; v.pos = { fRadius * cosf(a), fYLo, fRadius * sinf(a) };
                    v.normal = { 0.f, -1.f, 0.f }; v.uv = { 0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a) };
                    verts.push_back(v);
                }
                for (int i = 0; i < iSides; ++i)
                {
                    const unsigned int cur = ring + i;
                    const unsigned int nxt = ring + ((i + 1) % iSides);
                    inds.push_back(c); inds.push_back(cur); inds.push_back(nxt);
                }
            }

            return StaticCreateBindable<Mesh>(szTag, verts, inds);
        }
    }
}

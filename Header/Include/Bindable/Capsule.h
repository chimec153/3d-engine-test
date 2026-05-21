#pragma once
#include "Sphere.h"

namespace Engine
{
    // Static mesh helpers for a Y-axis capsule (two hemispheres + a cylinder
    // section). Layout mirrors Sphere's: top pole, then iRings hemisphere
    // rings, then two equator rings at y = +/- halfCyl, then iRings bottom
    // hemisphere rings, then the bottom pole. That keeps the connectivity
    // identical to a sphere with (2*iRings + 2) rings, so the index buffer
    // can simply forward to Sphere::CreateSphereIndex.
    //
    // Geometry uses the same 0.5 radius convention as Sphere, with the
    // capsule axis along +Y. fCylinderHeight is the straight section between
    // the two hemisphere caps (the equator rings sit at y = +/- height/2).
    class Capsule
    {
    public:
        template <typename T>
        static void CreateCapsuleVertex(int iRings, int iSector,
                                        float fCylinderHeight,
                                        std::vector<T>& pData)
        {
            pData.clear();

            const float fR      = 0.5f;
            const float halfCyl = fCylinderHeight * 0.5f;

            T tVertex = {};

            // Top pole.
            tVertex.pos.x = 0.f;
            tVertex.pos.y = fR + halfCyl;
            tVertex.pos.z = 0.f;
            pData.push_back(tVertex);

            // Top hemisphere rings — fA in (0, PI/2), excluding the equator.
            for (int i = 0; i < iRings; ++i)
            {
                const float fA =
                    PI / 2.f * (1.f - static_cast<float>(i + 1) /
                                       static_cast<float>(iRings + 1));
                const float r = cosf(fA) * fR;
                const float y = sinf(fA) * fR + halfCyl;
                for (int j = 0; j < iSector; ++j)
                {
                    const float fA2 = 2.f * PI / static_cast<float>(iSector) * j;
                    T v = {};
                    v.pos.x = cosf(fA2) * r;
                    v.pos.y = y;
                    v.pos.z = sinf(fA2) * r;
                    pData.push_back(v);
                }
            }

            // Top equator ring (cylinder top), y = +halfCyl.
            for (int j = 0; j < iSector; ++j)
            {
                const float fA2 = 2.f * PI / static_cast<float>(iSector) * j;
                T v = {};
                v.pos.x = cosf(fA2) * fR;
                v.pos.y = halfCyl;
                v.pos.z = sinf(fA2) * fR;
                pData.push_back(v);
            }

            // Bottom equator ring (cylinder bottom), y = -halfCyl.
            for (int j = 0; j < iSector; ++j)
            {
                const float fA2 = 2.f * PI / static_cast<float>(iSector) * j;
                T v = {};
                v.pos.x = cosf(fA2) * fR;
                v.pos.y = -halfCyl;
                v.pos.z = sinf(fA2) * fR;
                pData.push_back(v);
            }

            // Bottom hemisphere rings — fA in (-PI/2, 0), excluding the equator.
            for (int i = 0; i < iRings; ++i)
            {
                const float fA =
                    -PI / 2.f * static_cast<float>(i + 1) /
                                static_cast<float>(iRings + 1);
                const float r = cosf(fA) * fR;
                const float y = sinf(fA) * fR - halfCyl;
                for (int j = 0; j < iSector; ++j)
                {
                    const float fA2 = 2.f * PI / static_cast<float>(iSector) * j;
                    T v = {};
                    v.pos.x = cosf(fA2) * r;
                    v.pos.y = y;
                    v.pos.z = sinf(fA2) * r;
                    pData.push_back(v);
                }
            }

            // Bottom pole.
            tVertex.pos.x = 0.f;
            tVertex.pos.y = -fR - halfCyl;
            tVertex.pos.z = 0.f;
            pData.push_back(tVertex);
        }

        template <typename T>
        static void GetCapsuleVertexNormal(int /*iRings*/, int /*iSector*/,
                                           float fCylinderHeight,
                                           std::vector<T>& pData)
        {
            const float halfCyl = fCylinderHeight * 0.5f;
            for (size_t i = 0; i < pData.size(); ++i)
            {
                // Hemisphere centre is (0, +halfCyl, 0) for the top half and
                // (0, -halfCyl, 0) for the bottom. On the equator rings
                // pos.y = +/- halfCyl, so (pos.y - cy) = 0 and the normal
                // ends up purely radial — exactly what the cylinder wants.
                const float cy = pData[i].pos.y >= 0.f ? halfCyl : -halfCyl;
                float nx = pData[i].pos.x;
                float ny = pData[i].pos.y - cy;
                float nz = pData[i].pos.z;
                const float fLen = sqrtf(nx * nx + ny * ny + nz * nz);
                if (fLen > 0.f) { nx /= fLen; ny /= fLen; nz /= fLen; }
                pData[i].normal.x = nx;
                pData[i].normal.y = ny;
                pData[i].normal.z = nz;
            }
        }

        // Topology matches a sphere with (2*iRings + 2) rings — top pole fan,
        // a stack of quad strips between consecutive rings, bottom pole fan.
        static void CreateCapsuleIndex(int iRings, int iSectors,
                                       std::vector<unsigned int>& vecIndex)
        {
            Sphere::CreateSphereIndex(2 * iRings + 2, iSectors, vecIndex);
        }
    };
}

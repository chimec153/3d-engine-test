#pragma once

#include <memory>

namespace Engine { class Mesh; }

namespace Editor
{
    // Bake-time procedural fragment generator.
    //
    // Carves a convex base volume (box or icosphere) into N convex shards by
    // repeated random-plane clipping: a plane cuts the largest shard in two,
    // each straddling triangle is split, and the resulting cross-section hole
    // is capped with a triangle fan. Because the base is convex and every cut
    // is a plane, each shard stays convex and the cut cross-section is always a
    // single convex polygon, so the cap is watertight.
    //
    // Each shard becomes its own MeshContainer inside a single Engine::Mesh
    // (flat-shaded VertexStandard, faceted low-poly look). Shards are left in
    // the base volume's local space (NOT recentered), so the runtime shatter
    // effect can derive each shard's centroid at spawn time from its container
    // vertices. This is a pure offline tool: it only constructs the final Mesh,
    // it never runs at gameplay time.
    namespace FragmentGenerator
    {
        enum class BaseShape { Box, Sphere, Capsule, Cylinder };

        struct Params
        {
            BaseShape    eShape        = BaseShape::Box;
            int          iFragments    = 12;    // target shard count (clamped >= 1)
            unsigned int uSeed         = 1337;  // RNG seed for reproducible bakes
            float        fSize         = 1.f;   // box X/Z half-extent / sphere/capsule radius
            float        fBoxHeight    = 1.f;   // box Y half-extent (Box only; = fSize → cube)
            int          iSphereSubdiv = 1;     // icosphere subdivisions (Sphere only)

            // Capsule only. fCylHeight is the straight cylinder section (caps add
            // fSize each side, so total height = fCylHeight + 2*fSize).
            float        fCylHeight     = 0.4f;
            int          iCapsuleRings  = 4;    // latitude rings per hemisphere
            int          iCapsuleSectors = 12;  // longitude segments (also Cylinder)

            // Cylinder only. Radius = fSize, longitude segments = iCapsuleSectors;
            // fCylinderHeight is the full height along Y (caps are flat).
            float        fCylinderHeight = 2.f;
        };

        struct Result
        {
            std::shared_ptr<Engine::Mesh> pMesh;          // one container per shard
            int                           iFragments = 0; // shards actually produced
            int                           iVertices  = 0; // total verts across containers
            int                           iTriangles = 0; // total tris across containers
        };

        // Returns an empty Result (pMesh == nullptr) only if generation
        // collapsed to no geometry, which shouldn't happen for valid params.
        Result Generate(const Params& params);
    }
}

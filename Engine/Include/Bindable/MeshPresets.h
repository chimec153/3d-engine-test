#pragma once

#include "../Types.h"
#include "../Vector3.h"
#include <memory>

namespace Engine
{
    class Mesh;

    // Cached procedural meshes built on VertexStandard. Each call with
    // identical parameters returns the same Mesh instance via
    // BindableManager, so call sites don't write their own
    // "find-or-create + cache" wrapper. The wrappers just add caching
    // and a uniform API on top of the existing Sphere / Box / Capsule
    // helpers.
    //
    // Use for placeholder / projectile / debug-body geometry where a
    // hand-authored asset is overkill. Tags are auto-derived from
    // parameters so two callers passing the same args share one Mesh.
    namespace MeshPresets
    {
        ENGINE_DLL std::shared_ptr<Mesh> UnitSphere  (int iRings = 8, int iSectors = 16);
        ENGINE_DLL std::shared_ptr<Mesh> UnitBox     ();
        ENGINE_DLL std::shared_ptr<Mesh> UnitTriangle();

        // Capsule whose vertices are scaled by fScale and lifted by
        // fYLift after construction. Pass fScale=1, fYLift=0 for the
        // raw unit capsule.
        ENGINE_DLL std::shared_ptr<Mesh> UnitCapsule (int iRings, int iSectors,
                                                       float fCylHeight,
                                                       float fScale = 1.f,
                                                       float fYLift = 0.f);

        // Axis-aligned box with custom corner extents — useful when a
        // unit box at origin doesn't fit (e.g. an enemy footprint
        // sitting on y=0 with a specific x/z half-extent).
        ENGINE_DLL std::shared_ptr<Mesh> AxisBox     (const Vector3& vLo,
                                                       const Vector3& vHi);

        // Regular N-gon prism centred on the Y axis: an iSides-gon footprint of
        // radius fRadius extruded from y=fYLo to y=fYHi, with flat-shaded sides
        // and capped top/bottom. Side-count reads a tower's type at a glance
        // (prism3 = triangular .. prism8 = octagonal). Cached by parameters, so
        // every tower of the same shape shares one Mesh. CW-front + outward
        // normals, matching AxisBox.
        ENGINE_DLL std::shared_ptr<Mesh> RegularPrism(int iSides, float fRadius,
                                                       float fYLo, float fYHi);
    }
}

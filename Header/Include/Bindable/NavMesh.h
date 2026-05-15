#pragma once
#include "../Component/Component.h"
#include "../Navigation/Detour/DetourNavMeshBuilder.h"
#include "../Navigation/Detour/DetourCrowd.h"

class dtNavMesh;
class dtNavMeshQuery;
class dtCrowd;

namespace Engine
{
    class Agent;

    // Configuration for the Recast build pipeline. Defaults match the
    // legacy ImguiManager values so call sites that don't tune anything
    // behave the same as before. Units: cs/ch in world units, agent*
    // in world units, angles in degrees.
    //
    // Plain value struct — intentionally not marked ENGINE_DLL. With the
    // inline default-member initializers below, MSVC synthesises an
    // inline default ctor at every translation unit that includes this
    // header; tagging the struct dllimport would force the ctor to be
    // imported from Engine.dll instead, breaking client builds.
    struct NavMeshConfig
    {
        float fCellSize = 0.3f;
        float fCellHeight = 0.2f;
        float fAgentSlopeAngle = 60.f;
        float fAgentHeight = 2.f;
        float fAgentRadius = 0.6f;
        float fAgentClimb = 1.9f;
        float fMaxEdgeLen = 12.f;
        float fMaxEdgeError = 1.3f;
        float fRegionMinSize = 8.f;
        float fRegionMergeSize = 20.f;
        float fVertsPerPoly = 6.f;
        float fDetailSampleDist = 6.f;
        float fDetailSampleMaxError = 1.f;
    };

    // Phase B.4 — NavMesh migrated from Bindable to Component. Pure
    // navigation data structure; no GPU bindings. Bind() was a no-op.
    class ENGINE_DLL NavMesh :
        public Component
    {
    public:
        NavMesh(dtNavMeshCreateParams& tParam, float fAgentRadius, float fAgentHeight);
        NavMesh();
        virtual ~NavMesh() override;

    public:
        // Geometry-in / NavMesh-out builder. Runs the full Recast pipeline
        // (rasterize → compact → erode → distance field → regions →
        // contours → polymesh → polymesh detail → detour params) inside
        // one scope, frees every intermediate before returning, and hands
        // back a ready-to-use NavMesh.
        //
        //   vecPoint : flat float array, 3 components per vertex (x,y,z)
        //   vecTris  : flat int array, 3 indices per triangle
        //   vMax/Min : world-space AABB of the input mesh
        //   config   : Recast tuning knobs (defaults match the editor)
        //
        // Returns nullptr on any pipeline failure. Caller owns the result.
        static std::shared_ptr<NavMesh> Build(
            const std::vector<float>& vecPoint,
            const std::vector<int>& vecTris,
            const Vector3& vMax,
            const Vector3& vMin,
            const NavMeshConfig& config = NavMeshConfig());

        // Extract the Detour tile polygons as a renderable Mesh so the
        // navmesh can be drawn as a wireframe overlay on top of the scene.
        // Each polygon is fan-triangulated. Caller attaches the result to
        // a MeshRendererComponent with the WIREFRAME rasterizer state to
        // get the typical Recast/UE5-style nav debug look. Returns nullptr
        // if the underlying dtNavMesh hasn't been initialised yet.
        std::shared_ptr<class Mesh> CreateDebugMesh() const;

    private:
        class dtNavMesh* m_pNavMesh;
        class dtNavMeshQuery* m_pNavMeshQuery;
        unsigned char* m_pNavData;
        int m_iNavCount;
        float m_fAgentRadius;
        float m_fAgentHeight;
        class dtCrowd* m_pCrowd;
        std::unique_ptr<dtNavMeshCreateParams> m_pMeshCreateParams;
        std::list<std::shared_ptr<Agent>> m_AgentList;

    public:
        class dtNavMesh* GetNavMesh() const;
        int CreateAgent(const Vector3& pos, dtCrowdAgentParams& tParams);
        Vector3 GetAgentPos(int iIndex) const;
        Vector3 GetAgentVelocity(int iIndex) const;
        void SetTargetPos(int iIndex, const Vector3& pos);
        void DeleteAgent(int iIndex);
        void CreateNavMesh(dtNavMeshCreateParams& tParams);
        std::shared_ptr<Agent> CreateAgent(const std::string& strTag, std::shared_ptr<Transform> pTransform, const Vector3& vPos);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };


}
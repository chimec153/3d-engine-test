#pragma once
#include "../Core/Macro.h"
#include "../Vector3.h"
#include "BlockType.h"
#include "VoxelChunk.h"
#include <map>
#include <memory>
#include <tuple>

namespace Engine
{
    class Scene;
    class Layer;
    class GameObject;
    class MeshRendererComponent;

    // Phase V6 — single voxel raycast result. `hit` is the only field
    // worth reading when false. Face is the unit normal of the block
    // face that was struck (e.g. {1,0,0} means the ray hit the +X face).
    struct ENGINE_DLL VoxelRaycastHit
    {
        bool  hit;
        int   blockX, blockY, blockZ;   // world-coord block index
        int   faceX,  faceY,  faceZ;    // ±1 / 0 per axis — unit normal
        float distance;                 // t along the (unit) ray
    };

    // Phase V5 — VoxelWorld owns chunks lazily. SetBlock at any world
    // coord auto-creates the chunk that contains it (and spawns the
    // GameObject + MeshRenderer that draws it). Cross-chunk face
    // culling is wired through VoxelMesher::NeighbourLookup so chunk
    // borders don't leak interior faces.
    //
    // Coordinate spaces:
    //   world coord (int)  : block coord in the world (any sign, any range)
    //   chunk coord (int)  : floor(world / kSize)
    //   local  coord (int) : 0..kSize-1 inside its chunk
    class ENGINE_DLL VoxelWorld
    {
    public:
        VoxelWorld(Scene* pScene, const std::shared_ptr<Layer>& pLayer);
        ~VoxelWorld();

        VoxelWorld(const VoxelWorld&)            = delete;
        VoxelWorld& operator=(const VoxelWorld&) = delete;

        BlockType GetBlock(int wx, int wy, int wz) const;
        void      SetBlock(int wx, int wy, int wz, BlockType type);

        // Returns the highest solid-block Y in column (wx, wz), scanning
        // yMax down to yMin (inclusive). Returns yMin - 1 when the column
        // is fully empty in that range (used by the player as a "no
        // ground here" sentinel).
        int GetSurfaceHeight(int wx, int wz, int yMin, int yMax) const;

        // Phase V6 — Amanatides & Woo fast voxel traversal. `dir`
        // doesn't have to be unit length; `distance` in the result is
        // measured along the *normalised* direction. `maxDist` caps
        // the traversal. Stops on the first solid block.
        VoxelRaycastHit Raycast(const Vector3& origin,
                                const Vector3& dir,
                                float maxDist) const;

    private:
        // (cx, cy, cz). std::map needs operator< — std::tuple supplies one.
        using ChunkCoord = std::tuple<int, int, int>;

        struct ChunkRecord
        {
            std::unique_ptr<VoxelChunk>            pChunk;
            std::shared_ptr<GameObject>            pObj;
            std::shared_ptr<MeshRendererComponent> pMR;
        };

        Scene*                  m_pScene;
        std::shared_ptr<Layer>  m_pLayer;
        std::map<ChunkCoord, ChunkRecord> m_chunks;

        static ChunkCoord WorldToChunk(int wx, int wy, int wz);
        static void       WorldToLocal(int wx, int wy, int wz,
                                       int& lx, int& ly, int& lz);

        ChunkRecord*       FindChunk(const ChunkCoord& c);
        const ChunkRecord* FindChunk(const ChunkCoord& c) const;
        ChunkRecord*       EnsureChunk(const ChunkCoord& c);
        void               RebuildChunkMesh(const ChunkCoord& c);
    };
}

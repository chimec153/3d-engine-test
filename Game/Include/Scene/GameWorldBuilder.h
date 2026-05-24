#pragma once

namespace Engine { class VoxelWorld; }

namespace Client
{
    // Stamps the test scene's voxel layout — a 48×48 stone floor with a
    // single demo wall — onto a freshly created VoxelWorld. Keeping the
    // map in here (rather than inside GameScene::Init) means adding a
    // new map / level is a new builder, not a reshape of the scene.
    //
    // The Scene still owns VoxelWorld; this only writes blocks.
    class GameWorldBuilder
    {
    public:
        // Floor extents — exported so the EnemySpawner's clamp can match
        // without re-hardcoding 47 on its side.
        static constexpr int kFloorSize = 48;

        static void StampTestScene(Engine::VoxelWorld& world);
    };
}

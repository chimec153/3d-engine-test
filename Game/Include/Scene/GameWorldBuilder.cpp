#include "GameWorldBuilder.h"
#include "Voxel/VoxelWorld.h"
#include "Voxel/BlockType.h"
#include "../GameDefs.h"

namespace Client
{
    void GameWorldBuilder::StampTestScene(Engine::VoxelWorld& world)
    {
        // 48×48 stone slab at y=0 — the navigable floor.
        for (int x = 0; x < kFloorSize; ++x)
            for (int z = 0; z < kFloorSize; ++z)
                world.SetBlock(x, 0, z, Engine::BlockType::Stone);
    }
}

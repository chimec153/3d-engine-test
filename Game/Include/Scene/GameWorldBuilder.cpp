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

        // Demo wall: a stone strip at x=24 spanning z=14..34 at kWallY.
        // Enemies that spawn on one side of it and chase the player on
        // the other side will choose "break a wall block" if it beats
        // the detour cost.
        for (int z = 14; z <= 34; ++z)
            world.SetBlock(24, kWallY, z, Engine::BlockType::Stone);
    }
}

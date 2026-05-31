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

        // Perimeter wall at y=kWallY — encloses the arena so the player (and
        // enemies) can't leave the floor. kWallY is also the layer build/break
        // and bullet reflection operate on, so these read as normal walls.
        const int iLast = kFloorSize - 1;
        for (int x = 0; x < kFloorSize; ++x)
        {
            world.SetBlock(x, kWallY, 0,     Engine::BlockType::Stone);
            world.SetBlock(x, kWallY, iLast, Engine::BlockType::Stone);
        }
        for (int z = 0; z < kFloorSize; ++z)
        {
            world.SetBlock(0,     kWallY, z, Engine::BlockType::Stone);
            world.SetBlock(iLast, kWallY, z, Engine::BlockType::Stone);
        }
    }
}

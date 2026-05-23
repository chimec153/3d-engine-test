#pragma once

#include "../GameDefs.h"
#include <memory>

namespace Engine
{
    class Texture;
}

namespace Client
{
    // Debug HUD that prints a stack of per-frame counters to the top-right
    // of the screen as procedural 5x7 bitmap-font numbers. Currently shows
    //   row 0 (top)    : live enemy count (CountEnemies)
    //   row 1 (below)  : MeshRendererComponents drawn via the DrawInstanced
    //                    fast path this frame (RenderManager getter)
    // Add more rows by extending Render() — all share the one atlas.
    //
    // No external font asset is needed: a 50x7 RGBA atlas containing the
    // glyphs for digits 0..9 is generated procedurally on first Render.
    class GAME_DLL EnemyCountHUD
    {
    public:
        EnemyCountHUD() = default;
        ~EnemyCountHUD() = default;

        void Render();

    private:
        void EnsureAtlas();
        int  CountEnemies() const;
        // Renders an integer right-aligned at the given baseline Y in NDC.
        // baseY is the bottom of the digit row.
        void DrawNumber(int iValue, float fBaseY);

        std::shared_ptr<Engine::Texture> m_pAtlas;
    };
}

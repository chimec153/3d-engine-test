#pragma once

#include "../GameDefs.h"
#include "Vector3.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Engine
{
    class StructuredBuffer;
}

namespace Engine
{
    class Texture;
}

namespace Client
{
    // Floating combat text — a fixed-size pool of damage numbers that
    // drift up from the world position they were spawned at. We bake a
    // single DirectWrite glyph atlas (digits 0-9 + crit marker) at
    // start-up and draw each active number as N quads (one per digit)
    // sharing the same atlas SRV — no per-frame text rasterisation
    // even at high spawn rates.
    //
    // Singleton because both Enemy (spawn on hit) and the renderer
    // (UI pass) need to talk to the same pool without threading a
    // pointer through.
    class GAME_DLL DamageTextManager
    {
    public:
        static DamageTextManager* GetInst()
        {
            static DamageTextManager inst;
            return &inst;
        }

        // Bakes the atlas + reserves the pool. Safe to call multiple
        // times — only the first call does work.
        bool Init();

        // Take a free pool slot, seed it with the spawn-time data, mark
        // it active. If the pool is full the oldest (lowest age-left)
        // active slot is recycled — bias toward keeping fresh hits
        // visible.
        //
        // ownerHandle: opaque per-target id (e.g. reinterpret_cast of
        // an Enemy pointer). When non-zero, a recent active slot with
        // the same owner accumulates instead of spawning a new number,
        // so a burst on one enemy reads as a single growing total
        // rather than a stack of overlapping pop-ups.
        void Spawn(const Engine::Vector3& vWorldPos, int iValue, bool bCritical,
                   uintptr_t ownerHandle = 0);

        // Advance ages, drift positions, deactivate expired slots.
        void Update(float fDeltaTime);

        // Project every active slot to screen pixels and draw it as
        // digit quads using the bake atlas. Intended to be invoked
        // from RenderManager's UI custom-render queue.
        void Render();

        // Reset everything (atlas SRV survives — host stage-change).
        void Clear();

        // Release the atlas + instance buffer. Must run before
        // Graphics::DestroyInst() at shutdown — the Meyers singleton
        // would otherwise hold these D3D resources until static
        // destruction (after main returns), past the device's lifetime,
        // showing up as D3D11 "Live Object" warnings.
        void Shutdown();

    private:
        DamageTextManager() = default;
        ~DamageTextManager() = default;
        DamageTextManager(const DamageTextManager&) = delete;
        DamageTextManager& operator=(const DamageTextManager&) = delete;

        bool BakeAtlas();

        struct Particle
        {
            Engine::Vector3 vSpawnWorldPos;
            float    fVelocityY = 2.f;     // world units / sec, drift up
            float    fAge       = 0.f;
            float    fLifetime  = 1.f;     // seconds
            int      iValue     = 0;
            bool     bCritical  = false;
            float    fJitterX   = 0.f;     // small horizontal scatter (screen px)
            bool     bActive    = false;
            uintptr_t ownerHandle = 0;     // accumulation key (Enemy*)
        };

        // Per-glyph UV box + advance width baked by stb_truetype. Indexed
        // by ASCII codepoint − kFirstGlyph.
        struct GlyphBox
        {
            float u0, v0, u1, v1;          // texture-space UVs
            float fOffsetX, fOffsetY;      // pixel offset from baseline
            float fAdvance;                // pixel cursor advance after this glyph
            float fWidth, fHeight;         // quad pixel size
        };

        std::vector<Particle>            m_vecPool;
        std::shared_ptr<Engine::Texture> m_pAtlas;
        std::vector<GlyphBox>            m_vecGlyphs;
        float                            m_fAscent  = 0.f;
        bool                             m_bInitialised = false;

        // Per-frame instance scratchpad. Render fills this with one
        // GlyphInstance per quad it would have drawn (outlines +
        // main), then uploads + DrawInstanced in one shot. The shape
        // matches UI.fx's GlyphInstance — 12 floats / 48 bytes.
        struct InstanceCPU
        {
            float ndcRect[4];
            float uvRect[4];
            float tint[4];
        };
        std::vector<InstanceCPU>                 m_vecInstances;
        std::unique_ptr<Engine::StructuredBuffer> m_pInstanceBuffer;

        // Upper bound on instances per frame. Pool 64 × ~5 passes × 6
        // glyphs ≈ 1920; double for safety. ~190 KB GPU side.
        static constexpr int kMaxInstancesPerFrame = 4096;

        // ASCII printable range — covers digits, punctuation, K, M.
        // stb_truetype bakes them all in one call via stbtt_BakeFontBitmap.
        static constexpr int kFirstGlyph = 32;     // ' '
        static constexpr int kGlyphCount = 96;     // through '~'
        static constexpr int kAtlasW     = 512;
        static constexpr int kAtlasH     = 256;
        static constexpr int kPoolSize   = 64;

        // Accumulation: any hit landing on the same enemy within this
        // window adds to the existing slot instead of opening a new one.
        static constexpr float kAccumulateWindow = 0.35f;
    };
}

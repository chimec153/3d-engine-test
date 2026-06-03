#include "DamageText.h"

#include "Bindable/Texture.h"
#include "Bindable/BindableManager.h"
#include "Bindable/BindableRegistry.h"
#include "Bindable/ConstantBuffer.h"
#include "Bindable/VertexShader.h"
#include "Bindable/PixelShader.h"
#include "Bindable/Camera.h"
#include "Core/Graphics.h"
#include "Core/Window.h"
#include "Shader/StructuredBuffer.h"
#include "Matrix.h"
#include "Types.h"

// stb_truetype — single-file glyph rasteriser. Implementation lives in
// this .cpp only; the header is otherwise header-only.
#define STB_TRUETYPE_IMPLEMENTATION
#include "ThirdParty/stb_truetype.h"

#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <vector>

namespace Client
{
    namespace DamageText_detail
    {
        // Read an entire file into a heap buffer. Returns empty on
        // failure — caller falls back gracefully.
        std::vector<unsigned char> ReadAllBytes(const char* pPath)
        {
            std::vector<unsigned char> out;
            FILE* fp = nullptr;
            if (fopen_s(&fp, pPath, "rb") != 0 || !fp) return out;
            std::fseek(fp, 0, SEEK_END);
            long len = std::ftell(fp);
            if (len > 0)
            {
                out.resize(static_cast<size_t>(len));
                std::fseek(fp, 0, SEEK_SET);
                std::fread(out.data(), 1, static_cast<size_t>(len), fp);
            }
            std::fclose(fp);
            return out;
        }
    }

    bool DamageTextManager::Init()
    {
        if (m_bInitialised) return true;

        m_vecPool.assign(kPoolSize, Particle{});
        if (!BakeAtlas()) return false;

        // Reserve the instance scratch + create the GPU-side dynamic
        // structured buffer. USAGE_DYNAMIC so StructuredBuffer's
        // WriteData (Map WRITE_DISCARD) works. SRV-only — VS reads at
        // t1, no compute write-back path.
        m_vecInstances.reserve(kMaxInstancesPerFrame);
        m_pInstanceBuffer = std::make_unique<Engine::StructuredBuffer>(
            kMaxInstancesPerFrame, static_cast<int>(sizeof(InstanceCPU)),
            nullptr, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE);
        if (!m_pInstanceBuffer) return false;

        // Piggy-back on the Bindable shutdown registry: our atlas +
        // instance buffer are D3D11-backed and the Meyers singleton
        // would otherwise outlive Graphics. Registering here means
        // both Client and Editor mains release us automatically via
        // their existing BindableRegistry::DestroyAll() call — no
        // per-app wiring needed.
        Engine::BindableRegistry::Register(
            []() { DamageTextManager::GetInst()->Shutdown(); });

        m_bInitialised = true;
        return true;
    }

    bool DamageTextManager::BakeAtlas()
    {
        // Read a system font. Arial ships with every Windows install so
        // the data file is reliably available at this path. Easy to
        // swap to a bundled .ttf later — change this one line.
        auto vecFont = DamageText_detail::ReadAllBytes("C:\\Windows\\Fonts\\arialbd.ttf");
        if (vecFont.empty())
            vecFont = DamageText_detail::ReadAllBytes("C:\\Windows\\Fonts\\arial.ttf");
        if (vecFont.empty()) return false;

        // stb_truetype face init — used both for the bake and for the
        // ascent/descent metrics we need to baseline-align glyphs.
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, vecFont.data(),
            stbtt_GetFontOffsetForIndex(vecFont.data(), 0)))
            return false;

        const float fFontPxH = 56.f;   // bake size; render-time scaling further tunes on-screen size
        const float fScale = stbtt_ScaleForPixelHeight(&info, fFontPxH);
        int iAscent = 0, iDescent = 0, iLineGap = 0;
        stbtt_GetFontVMetrics(&info, &iAscent, &iDescent, &iLineGap);
        m_fAscent = iAscent * fScale;

        // Bake printable ASCII (32..127) into a single R8 atlas.
        std::vector<unsigned char> atlasR8(kAtlasW * kAtlasH, 0);
        std::vector<stbtt_bakedchar> chardata(kGlyphCount);
        if (stbtt_BakeFontBitmap(vecFont.data(), 0, fFontPxH,
                atlasR8.data(), kAtlasW, kAtlasH,
                kFirstGlyph, kGlyphCount, chardata.data()) <= 0)
        {
            // Returned <=0 means atlas couldn't hold everything — bigger
            // atlas would fix it. Bail rather than render garbage.
            return false;
        }

        // R8 → RGBA8 expansion. RGB = 255 (so PS_UI fallback still
        // shows white if used), A = glyph coverage from stb.
        std::vector<unsigned char> atlasRGBA(kAtlasW * kAtlasH * 4);
        for (int i = 0; i < kAtlasW * kAtlasH; ++i)
        {
            atlasRGBA[i * 4 + 0] = 255;
            atlasRGBA[i * 4 + 1] = 255;
            atlasRGBA[i * 4 + 2] = 255;
            atlasRGBA[i * 4 + 3] = atlasR8[i];
        }

        m_pAtlas = Engine::StaticCreateBindable<Engine::Texture>("DamageTextAtlas");
        if (!m_pAtlas) return false;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem     = atlasRGBA.data();
        init.SysMemPitch = kAtlasW * 4;
        if (!m_pAtlas->CreateTexture(kAtlasW, kAtlasH,
                DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init))
            return false;
        if (!m_pAtlas->CreateShaderResourceView(
                DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1))
            return false;

        // Cache per-glyph UV + offsets. stb gives us pixel rects + a
        // pixel cursor advance; we normalise the rect to UVs so the
        // render path only does float math.
        m_vecGlyphs.assign(kGlyphCount, GlyphBox{});
        for (int i = 0; i < kGlyphCount; ++i)
        {
            const auto& c = chardata[i];
            GlyphBox& g = m_vecGlyphs[i];
            g.u0       = static_cast<float>(c.x0) / kAtlasW;
            g.v0       = static_cast<float>(c.y0) / kAtlasH;
            g.u1       = static_cast<float>(c.x1) / kAtlasW;
            g.v1       = static_cast<float>(c.y1) / kAtlasH;
            g.fOffsetX = c.xoff;
            g.fOffsetY = c.yoff;
            g.fAdvance = c.xadvance;
            g.fWidth   = static_cast<float>(c.x1 - c.x0);
            g.fHeight  = static_cast<float>(c.y1 - c.y0);
        }
        return true;
    }

    void DamageTextManager::Spawn(const Engine::Vector3& vWorldPos,
                                  int iValue, bool bCritical,
                                  uintptr_t ownerHandle, bool bHeal)
    {
        if (!m_bInitialised || iValue <= 0) return;

        // Accumulation pass — fold into a recent slot on the same target.
        if (ownerHandle != 0)
        {
            for (auto& p : m_vecPool)
            {
                if (p.bActive && p.ownerHandle == ownerHandle
                    && p.fAge < kAccumulateWindow)
                {
                    p.iValue += iValue;
                    p.fAge = 0.f;                  // restart fade
                    p.vSpawnWorldPos = vWorldPos;  // follow the latest hit
                    p.bCritical = p.bCritical || bCritical;
                    p.bHeal = bHeal;
                    return;
                }
            }
        }

        // Find a free slot; if none, kick the oldest active.
        int iFree = -1, iOldest = 0;
        float fOldestAge = -1.f;
        for (int i = 0; i < static_cast<int>(m_vecPool.size()); ++i)
        {
            if (!m_vecPool[i].bActive) { iFree = i; break; }
            if (m_vecPool[i].fAge > fOldestAge)
            {
                fOldestAge = m_vecPool[i].fAge;
                iOldest = i;
            }
        }
        if (iFree < 0) iFree = iOldest;

        Particle& p = m_vecPool[iFree];
        p.vSpawnWorldPos = vWorldPos;
        p.fVelocityY = bCritical ? 2.8f : 2.2f;
        p.fAge       = 0.f;
        p.fLifetime  = bCritical ? 1.2f : 0.9f;
        p.iValue     = iValue;
        p.bCritical  = bCritical;
        p.bHeal      = bHeal;
        p.fJitterX   = (static_cast<float>(std::rand() % 17) - 8.f);
        p.bActive    = true;
        p.ownerHandle = ownerHandle;
    }

    void DamageTextManager::Update(float fDeltaTime)
    {
        for (auto& p : m_vecPool)
        {
            if (!p.bActive) continue;
            p.fAge += fDeltaTime;
            if (p.fAge >= p.fLifetime) { p.bActive = false; continue; }
        }
    }

    void DamageTextManager::Clear()
    {
        for (auto& p : m_vecPool) p.bActive = false;
    }

    void DamageTextManager::Shutdown()
    {
        m_pAtlas.reset();
        m_pInstanceBuffer.reset();
        m_vecPool.clear();
        m_vecGlyphs.clear();
        m_vecInstances.clear();
        m_bInitialised = false;
    }

    namespace
    {
        // Convert a screen-pixel quad (top-left + size) into the
        // ndcRect a GlyphInstance carries: ((ndcX, ndcY) = bottom-left
        // in NDC, (ndcW, ndcH) = NDC size). Matches the UV/quad-vertex
        // convention from UI.fx's static UIQuad path.
        inline void PxToNdcRect(float fX, float fY, float fW, float fH,
                                float fSW, float fSH, float out[4])
        {
            out[0] = fX / fSW * 2.f - 1.f;
            out[1] = 1.f - (fY + fH) / fSH * 2.f;
            out[2] = fW / fSW * 2.f;
            out[3] = fH / fSH * 2.f;
        }
    }

    void DamageTextManager::Render()
    {
        if (!m_bInitialised || !m_pAtlas || !m_pInstanceBuffer) return;

        auto pVS = Engine::StaticFindBindable<Engine::VertexShader>("UIVSInst");
        auto pPS = Engine::StaticFindBindable<Engine::PixelShader> ("UIPSInst");
        if (!pVS || !pPS) return;

        auto* pCamera = Engine::Graphics::GetInst()->GetCamera(Engine::CAMERA_TYPE::NORMAL).get();
        if (!pCamera) return;

        auto pDC = Engine::Graphics::GetInst()->GetDeviceContext();
        const float fSW = static_cast<float>(Engine::Window::GetInst()->GetWidth());
        const float fSH = static_cast<float>(Engine::Window::GetInst()->GetHeight());

        // 4-direction outline offsets in screen pixels. Pass 0..3 are
        // black edge copies, pass 4 is the coloured main glyph drawn
        // on top — same effect as the per-quad draw path, just every
        // quad lives as a GlyphInstance fed to one DrawInstanced.
        struct Off { float dx, dy; };
        const Off kOutline[4] = { {-1.f,0.f}, {1.f,0.f}, {0.f,-1.f}, {0.f,1.f} };

        m_vecInstances.clear();

        for (const auto& p : m_vecPool)
        {
            if (!p.bActive) continue;

            const float t  = p.fAge / p.fLifetime;
            const float tE = 1.f - (1.f - t) * (1.f - t);
            Engine::Vector3 vPos = p.vSpawnWorldPos;
            vPos.y += p.fVelocityY * tE * p.fLifetime * 0.5f;

            float fPxX, fPxY, fPxW;
            if (!pCamera->WorldToScreen(vPos, fPxX, fPxY, fPxW)) continue;

            const float fBaseSize = 24.f;
            const float fBaseGlyphH = p.bCritical ? fBaseSize * 1.5f: fBaseSize;
            const float fDistScale = (std::min)(2.f, (std::max)(0.4f, 18.f / fPxW));
            const float fPop = (t < 0.15f)
                ? 1.f + 0.4f * (1.f - (t / 0.15f) * (t / 0.15f))
                : 1.f;
            const float fGlyphPxH = fBaseGlyphH * fDistScale * fPop;
            const float fBakeToScreen = fGlyphPxH / 56.f;

            const float fFadeStart = 0.7f;
            const float fAlpha = (t < fFadeStart)
                ? 1.f
                : (std::max)(0.f, 1.f - (t - fFadeStart) / (1.f - fFadeStart));

            char buf[16];
            int  iLen = std::snprintf(buf, sizeof(buf),
                                      p.bHeal ? "+%d" : "%d", p.iValue);
            if (iLen <= 0) continue;

            float fTotalW = 0.f;
            for (int i = 0; i < iLen; ++i)
            {
                const int idx = static_cast<int>(buf[i]) - kFirstGlyph;
                if (idx < 0 || idx >= kGlyphCount) continue;
                fTotalW += m_vecGlyphs[idx].fAdvance * fBakeToScreen;
            }

            const float fStartX = fPxX - fTotalW * 0.5f + p.fJitterX;
            const float fBaselineY = fPxY - fGlyphPxH * 0.3f;

            float vMain[4] = { 1.f, 0.95f, 0.85f, fAlpha };
            if (p.bCritical) { vMain[0] = 1.f; vMain[1] = 0.25f; vMain[2] = 0.15f; }
            if (p.bHeal)     { vMain[0] = 0.3f; vMain[1] = 1.f; vMain[2] = 0.45f; }
            const float vEdge[4] = { 0.f, 0.f, 0.f, fAlpha };

            // Emit 5 passes × N glyphs worth of GlyphInstance entries
            // — same data the per-quad path used to push individually,
            // now bundled for a single DrawInstanced. Outline first so
            // it draws behind the coloured main pass (we'll reverse-
            // append the main pass last → highest instance index draws
            // last; primitive-strip draw order is deterministic per
            // instance).
            for (int pass = 0; pass < 5; ++pass)
            {
                const bool bOutline = (pass < 4);
                const float dx = bOutline ? kOutline[pass].dx : 0.f;
                const float dy = bOutline ? kOutline[pass].dy : 0.f;
                const float* tint = bOutline ? vEdge : vMain;

                float fCursorX = fStartX + dx;
                for (int i = 0; i < iLen; ++i)
                {
                    const int idx = static_cast<int>(buf[i]) - kFirstGlyph;
                    if (idx < 0 || idx >= kGlyphCount) continue;
                    const GlyphBox& g = m_vecGlyphs[idx];

                    const float fW = g.fWidth  * fBakeToScreen;
                    const float fH = g.fHeight * fBakeToScreen;
                    const float fX = fCursorX + g.fOffsetX * fBakeToScreen;
                    const float fY = fBaselineY + g.fOffsetY * fBakeToScreen + dy;

                    if (static_cast<int>(m_vecInstances.size()) >= kMaxInstancesPerFrame)
                    {
                        // Hit the capacity — drop the remaining glyphs
                        // for this frame so we don't overflow the GPU
                        // buffer. Rare; bump kMaxInstancesPerFrame if
                        // this trips during play.
                        i = iLen;
                        pass = 5;
                        break;
                    }
                    InstanceCPU ins{};
                    PxToNdcRect(fX, fY, fW, fH, fSW, fSH, ins.ndcRect);
                    ins.uvRect[0] = g.u0; ins.uvRect[1] = g.v0;
                    ins.uvRect[2] = g.u1; ins.uvRect[3] = g.v1;
                    ins.tint[0] = tint[0]; ins.tint[1] = tint[1];
                    ins.tint[2] = tint[2]; ins.tint[3] = tint[3];
                    m_vecInstances.push_back(ins);

                    fCursorX += g.fAdvance * fBakeToScreen;
                }
            }
        }

        const int iInstanceCount = static_cast<int>(m_vecInstances.size());
        if (iInstanceCount <= 0) return;

        // One CPU → GPU upload, one draw. Replaces the prior per-glyph
        // SetCBuffer + Draw(4, 0) cascade.
        m_pInstanceBuffer->WriteData(m_vecInstances.data(), iInstanceCount);

        pVS->Bind();
        pPS->Bind();
        m_pAtlas->Bind();
        m_pInstanceBuffer->SetSRV(1);

        pDC->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        pDC->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        pDC->IASetInputLayout(nullptr);
        pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        pDC->DrawInstanced(4, iInstanceCount, 0, 0);

        // Hard tear-down — unbind SRVs and shaders so the next pass
        // starts from a clean slot 0/1, just like the old path did.
        m_pInstanceBuffer->ResetSRV(1);
        ID3D11ShaderResourceView* pNullSRV[1] = { nullptr };
        pDC->PSSetShaderResources(0, 1, pNullSRV);
        pDC->VSSetShaderResources(0, 1, pNullSRV);
        pDC->VSSetShader(nullptr, nullptr, 0);
        pDC->PSSetShader(nullptr, nullptr, 0);
        Engine::Graphics::GetInst()->ResetBindCache();
    }
}

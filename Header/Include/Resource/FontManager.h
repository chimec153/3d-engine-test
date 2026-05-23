#pragma once

#include "../Core/Macro.h"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine
{
    class Font;

    // Process-wide font + text-rendering registry. Holds the shared
    // Direct2D / DirectWrite / WIC factories used by the Text class for
    // off-screen glyph rasterisation, and caches Font instances by tag.
    //
    // Same singleton pattern as ResourceManager — Init() is lazy
    // (CoInitializeEx + factory creation runs on first CreateFont /
    // factory accessor). Destroyed explicitly by the host .exe at
    // shutdown (DestroyInst), same shape ResourceManager / Graphics use.
    class ENGINE_DLL FontManager
    {
    private:
        FontManager() = default;
        ~FontManager() = default;

        static FontManager* m_pInst;

    public:
        static FontManager* GetInst()
        {
            if (!m_pInst)
                m_pInst = new FontManager;
            return m_pInst;
        }
        static void DestroyInst()
        {
            if (m_pInst) { delete m_pInst; m_pInst = nullptr; }
        }

        // Get or create a font keyed by `strTag`. A second call with the
        // same tag returns the cached instance and ignores the size /
        // weight arguments — callers wanting a different size should
        // use a different tag.
        std::shared_ptr<Font> CreateFont(const std::string& strTag,
                                        const std::wstring& strFamily,
                                        float fSize,
                                        DWRITE_FONT_WEIGHT eWeight = DWRITE_FONT_WEIGHT_NORMAL);

        std::shared_ptr<Font> FindFont(const std::string& strTag) const;

        // Factory accessors. Text uses these directly to build per-string
        // bitmaps; external callers can use them if they need ad-hoc
        // D2D drawing into a software bitmap target.
        ID2D1Factory*       GetD2DFactory();
        IDWriteFactory*     GetDWriteFactory();
        IWICImagingFactory* GetWICFactory();

    private:
        // Lazy factory initialisation. Returns false if any of the COM
        // factories fail; in that case Text rendering becomes a no-op.
        bool EnsureFactories();

        Microsoft::WRL::ComPtr<ID2D1Factory>       m_pD2DFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory>     m_pDWriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory> m_pWICFactory;

        std::unordered_map<std::string, std::shared_ptr<Font>> m_mapFont;
    };
}

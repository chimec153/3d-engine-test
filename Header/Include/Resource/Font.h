#pragma once

#include "../Core/Macro.h"
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

namespace Engine
{
    // Thin wrapper around DirectWrite's IDWriteTextFormat. FontManager
    // owns these (one per (family, size, weight) tuple); Text instances
    // hold them via shared_ptr.
    //
    // Construction goes through FontManager::CreateFont — the format
    // needs the shared IDWriteFactory the manager keeps alive, so
    // exposing a public constructor on Font would just leak that
    // dependency back out.
    class ENGINE_DLL Font
    {
    public:
        Font(const std::wstring& strFamily,
             float fSize,
             DWRITE_FONT_WEIGHT eWeight,
             Microsoft::WRL::ComPtr<IDWriteTextFormat> pFormat);
        ~Font() = default;

        IDWriteTextFormat* GetTextFormat() const { return m_pTextFormat.Get(); }

        const std::wstring& GetFamily() const { return m_strFamily; }
        float               GetSize()   const { return m_fSize; }
        DWRITE_FONT_WEIGHT  GetWeight() const { return m_eWeight; }

    private:
        std::wstring                              m_strFamily;
        float                                     m_fSize    = 12.f;
        DWRITE_FONT_WEIGHT                        m_eWeight  = DWRITE_FONT_WEIGHT_NORMAL;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> m_pTextFormat;
    };
}

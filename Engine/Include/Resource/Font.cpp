#include "Font.h"

namespace Engine
{
    Font::Font(const std::wstring& strFamily,
               float fSize,
               DWRITE_FONT_WEIGHT eWeight,
               Microsoft::WRL::ComPtr<IDWriteTextFormat> pFormat)
        : m_strFamily(strFamily)
        , m_fSize(fSize)
        , m_eWeight(eWeight)
        , m_pTextFormat(std::move(pFormat))
    {
    }
}

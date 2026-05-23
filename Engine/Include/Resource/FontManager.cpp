#include "FontManager.h"
#include "Font.h"
#include <Windows.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace Engine
{
    FontManager* FontManager::m_pInst = nullptr;

    bool FontManager::EnsureFactories()
    {
        if (m_pD2DFactory && m_pDWriteFactory && m_pWICFactory) return true;

        // WIC needs COM. Calling CoInitializeEx after the main thread
        // already initialised COM just returns S_FALSE / RPC_E_CHANGED_MODE
        // — harmless. We never CoUninitialize because the factories live
        // for the lifetime of the manager (and the .exe).
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        if (!m_pD2DFactory)
        {
            HRESULT hr = D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                __uuidof(ID2D1Factory),
                reinterpret_cast<void**>(m_pD2DFactory.GetAddressOf()));
            if (FAILED(hr)) return false;
        }
        if (!m_pDWriteFactory)
        {
            HRESULT hr = DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(m_pDWriteFactory.GetAddressOf()));
            if (FAILED(hr)) return false;
        }
        if (!m_pWICFactory)
        {
            HRESULT hr = CoCreateInstance(
                CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                __uuidof(IWICImagingFactory),
                reinterpret_cast<void**>(m_pWICFactory.GetAddressOf()));
            if (FAILED(hr)) return false;
        }
        return true;
    }

    std::shared_ptr<Font> FontManager::CreateFont(const std::string& strTag,
                                                  const std::wstring& strFamily,
                                                  float fSize,
                                                  DWRITE_FONT_WEIGHT eWeight)
    {
        auto it = m_mapFont.find(strTag);
        if (it != m_mapFont.end()) return it->second;

        if (!EnsureFactories()) return nullptr;

        Microsoft::WRL::ComPtr<IDWriteTextFormat> pFormat;
        HRESULT hr = m_pDWriteFactory->CreateTextFormat(
            strFamily.c_str(), nullptr,
            eWeight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            fSize, L"en-us", pFormat.GetAddressOf());
        if (FAILED(hr) || !pFormat) return nullptr;

        auto pFont = std::make_shared<Font>(strFamily, fSize, eWeight, std::move(pFormat));
        m_mapFont.emplace(strTag, pFont);
        return pFont;
    }

    std::shared_ptr<Font> FontManager::FindFont(const std::string& strTag) const
    {
        auto it = m_mapFont.find(strTag);
        return it == m_mapFont.end() ? nullptr : it->second;
    }

    ID2D1Factory*       FontManager::GetD2DFactory()    { EnsureFactories(); return m_pD2DFactory.Get(); }
    IDWriteFactory*     FontManager::GetDWriteFactory() { EnsureFactories(); return m_pDWriteFactory.Get(); }
    IWICImagingFactory* FontManager::GetWICFactory()    { EnsureFactories(); return m_pWICFactory.Get(); }
}

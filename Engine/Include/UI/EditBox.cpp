#include "EditBox.h"
#include "Button.h"
#include "../Resource/Text.h"
#include "../Input/Input.h"
#include "../Bindable/Texture.h"
#include "../Bindable/BindableManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

namespace Engine
{
    // Named (not anonymous) so the unity/jumbo build doesn't merge this file's
    // PackABGR/SolidTex with another UI .cpp's into a redefinition.
    namespace EditBox_detail
    {
        unsigned int PackABGR(unsigned int uRGB, unsigned int uAlpha)
        {
            const unsigned int r = (uRGB >> 16) & 0xFF;
            const unsigned int g = (uRGB >>  8) & 0xFF;
            const unsigned int b = (uRGB      ) & 0xFF;
            return (uAlpha << 24) | (b << 16) | (g << 8) | r;
        }

        std::shared_ptr<Texture> SolidTex(const char* szTag, unsigned int uRGB, unsigned int uAlpha)
        {
            if (auto p = StaticFindBindable<Texture>(szTag)) return p;
            auto pNew = StaticCreateBindable<Texture>(szTag);
            if (!pNew) return nullptr;
            unsigned int uColor = PackABGR(uRGB, uAlpha);
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem     = &uColor;
            init.SysMemPitch = 4;
            pNew->CreateTexture(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, &init);
            pNew->CreateShaderResourceView(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
            return pNew;
        }

        // Digit DIK -> char (main row + numpad).
        struct KeyChar { unsigned char dik; wchar_t ch; };
        const KeyChar kDigits[] = {
            { DIK_1, L'1' }, { DIK_2, L'2' }, { DIK_3, L'3' }, { DIK_4, L'4' }, { DIK_5, L'5' },
            { DIK_6, L'6' }, { DIK_7, L'7' }, { DIK_8, L'8' }, { DIK_9, L'9' }, { DIK_0, L'0' },
            { DIK_NUMPAD1, L'1' }, { DIK_NUMPAD2, L'2' }, { DIK_NUMPAD3, L'3' },
            { DIK_NUMPAD4, L'4' }, { DIK_NUMPAD5, L'5' }, { DIK_NUMPAD6, L'6' },
            { DIK_NUMPAD7, L'7' }, { DIK_NUMPAD8, L'8' }, { DIK_NUMPAD9, L'9' },
            { DIK_NUMPAD0, L'0' },
        };

        const unsigned char kPolledKeys[] = {
            DIK_1, DIK_2, DIK_3, DIK_4, DIK_5, DIK_6, DIK_7, DIK_8, DIK_9, DIK_0,
            DIK_NUMPAD0, DIK_NUMPAD1, DIK_NUMPAD2, DIK_NUMPAD3, DIK_NUMPAD4,
            DIK_NUMPAD5, DIK_NUMPAD6, DIK_NUMPAD7, DIK_NUMPAD8, DIK_NUMPAD9,
            DIK_PERIOD, DIK_DECIMAL, DIK_MINUS, DIK_SUBTRACT,
            DIK_BACK, DIK_RETURN, DIK_NUMPADENTER,
        };

        // Text-mode keys: A-Z (case from Shift), space, and the Shift modifiers.
        // Registered in addition to kPolledKeys so a text EditBox can type names.
        struct LetterKey { unsigned char dik; wchar_t lower; wchar_t upper; };
        const LetterKey kLetters[] = {
            { DIK_A, L'a', L'A' }, { DIK_B, L'b', L'B' }, { DIK_C, L'c', L'C' },
            { DIK_D, L'd', L'D' }, { DIK_E, L'e', L'E' }, { DIK_F, L'f', L'F' },
            { DIK_G, L'g', L'G' }, { DIK_H, L'h', L'H' }, { DIK_I, L'i', L'I' },
            { DIK_J, L'j', L'J' }, { DIK_K, L'k', L'K' }, { DIK_L, L'l', L'L' },
            { DIK_M, L'm', L'M' }, { DIK_N, L'n', L'N' }, { DIK_O, L'o', L'O' },
            { DIK_P, L'p', L'P' }, { DIK_Q, L'q', L'Q' }, { DIK_R, L'r', L'R' },
            { DIK_S, L's', L'S' }, { DIK_T, L't', L'T' }, { DIK_U, L'u', L'U' },
            { DIK_V, L'v', L'V' }, { DIK_W, L'w', L'W' }, { DIK_X, L'x', L'X' },
            { DIK_Y, L'y', L'Y' }, { DIK_Z, L'z', L'Z' },
        };
        const unsigned char kTextKeys[] = {
            DIK_A, DIK_B, DIK_C, DIK_D, DIK_E, DIK_F, DIK_G, DIK_H, DIK_I, DIK_J,
            DIK_K, DIK_L, DIK_M, DIK_N, DIK_O, DIK_P, DIK_Q, DIK_R, DIK_S, DIK_T,
            DIK_U, DIK_V, DIK_W, DIK_X, DIK_Y, DIK_Z, DIK_SPACE, DIK_LSHIFT, DIK_RSHIFT,
        };
        const size_t kMaxNameLen = 16;

        constexpr unsigned int kBoxRGB      = 0x202830;    // input panel
        constexpr unsigned int kBoxFocusRGB = 0x33424E;    // brighter when focused
        constexpr unsigned int kBoxDimRGB   = 0x171B1F;
        constexpr unsigned int kTextOn      = 0xFFFFFFFFu; // RGBA
        constexpr unsigned int kTextOff     = 0x9AA0A6FFu;

        const size_t kMaxTypeLen = 8;
    }

    EditBox::EditBox()
        : UIControl()
    {
        Component::SetComponentType(COMPONENT_TYPE::NONE);
    }

    bool EditBox::Init()
    {
        using namespace EditBox_detail;
        if (!UIControl::Init()) return false;

        // Register the digit / edit keys once (CInput::Init only registers the
        // gameplay keys). Guard against duplicates with FindKey.
        if (auto* pInput = CInput::GetInst())
        {
            for (unsigned char k : kPolledKeys)
                if (pInput->FindKey(k) == nullptr)
                    pInput->AddKey(k);
            for (unsigned char k : kTextKeys)
                if (pInput->FindKey(k) == nullptr)
                    pInput->AddKey(k);
        }

        m_pBox = CreateComponent<Button>("editbox_bg");
        if (m_pBox) m_pBox->SetTexture(SolidTex("editbox_bg", kBoxRGB, 0xFF));

        m_pText = CreateComponent<Text>("editbox_text");
        if (m_pText)
        {
            m_pText->SetColor(kTextOn);
            m_pText->SetHAlign(Text::HAlign::Center);
            m_pText->SetVAlign(Text::VAlign::Center);
        }

        m_strBuf = Format(m_fValue);
        return true;
    }

    void EditBox::SetFont(const std::shared_ptr<Font>& pFont)
    {
        if (m_pText) m_pText->SetFont(pFont);
        m_strShown.clear();   // force the next UpdateText to push the string
        Relayout();
    }

    void EditBox::SetBoxRect(float fX, float fY, float fW, float fH)
    {
        m_fRect[0] = fX; m_fRect[1] = fY; m_fRect[2] = fW; m_fRect[3] = fH;
        Relayout();
    }

    void EditBox::SetValue(float fValue)
    {
        m_bTyping = false;
        m_fValue  = fValue;
        m_strBuf  = Format(fValue);
        UpdateText();   // text only — avoid re-issuing SetRect (a Text re-bake) per drag frame
    }

    void EditBox::SetText(const std::wstring& wStr)
    {
        m_bTyping = false;
        m_strBuf  = wStr;
        UpdateText();
    }

    void EditBox::SetEnabled(bool bEnabled)
    {
        using namespace EditBox_detail;
        m_bEnabled = bEnabled;
        if (!bEnabled) { m_bFocused = false; }
        if (m_pText) m_pText->SetColor(bEnabled ? kTextOn : kTextOff);
        ApplyFocusVisual();
    }

    std::wstring EditBox::Format(float fValue) const
    {
        wchar_t buf[32];
        const int dec = m_iDecimals < 0 ? 0 : m_iDecimals;
        std::swprintf(buf, 32, L"%.*f", dec, fValue);
        return buf;
    }

    void EditBox::Relayout()
    {
        const float x = m_fRect[0], y = m_fRect[1], w = m_fRect[2], h = m_fRect[3];
        if (w <= 0.f || h <= 0.f) return;

        if (m_pBox) m_pBox->SetRect(x, y, w, h);
        // Text inset slightly from the box edges.
        if (m_pText) m_pText->SetRect(x + w * 0.06f, y, w * 0.88f, h);
        UpdateText();
    }

    void EditBox::UpdateText()
    {
        if (!m_pText) return;
        // A static caret while focused signals which box is taking input.
        const std::wstring disp = m_bFocused ? (m_strBuf + L"|") : m_strBuf;
        if (disp != m_strShown)
        {
            m_pText->SetString(disp);
            m_strShown = disp;
        }
    }

    void EditBox::ApplyFocusVisual()
    {
        using namespace EditBox_detail;
        if (!m_pBox) return;
        const unsigned int rgb = !m_bEnabled ? kBoxDimRGB
                               : (m_bFocused ? kBoxFocusRGB : kBoxRGB);
        const char* tag = !m_bEnabled ? "editbox_bg_dim"
                        : (m_bFocused ? "editbox_bg_focus" : "editbox_bg");
        m_pBox->SetTexture(SolidTex(tag, rgb, 0xFF));
        m_bFocusShown = m_bFocused;
    }

    void EditBox::Commit()
    {
        m_bTyping = false;
        if (m_bTextMode) { if (m_fnOnCommitText) m_fnOnCommitText(m_strBuf); return; }
        if (m_fnOnCommit) m_fnOnCommit(m_fValue);
    }

    void EditBox::PollText()
    {
        using namespace EditBox_detail;
        auto* pInput = CInput::GetInst();
        if (!pInput) return;

        bool bChanged = false;
        const bool bShift = pInput->IsKey(CInput::KEY_STATE::PRESS, DIK_LSHIFT)
                         || pInput->IsKey(CInput::KEY_STATE::PRESS, DIK_RSHIFT);

        for (const LetterKey& lk : kLetters)
            if (pInput->IsKey(CInput::KEY_STATE::DOWN, lk.dik) && m_strBuf.size() < kMaxNameLen)
            {
                m_strBuf += (bShift ? lk.upper : lk.lower);
                bChanged = true;
            }
        for (const KeyChar& kc : kDigits)
            if (pInput->IsKey(CInput::KEY_STATE::DOWN, kc.dik) && m_strBuf.size() < kMaxNameLen)
            {
                m_strBuf += kc.ch;
                bChanged = true;
            }
        if (pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_SPACE) && m_strBuf.size() < kMaxNameLen)
        {
            m_strBuf += L' ';
            bChanged = true;
        }
        if (pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_BACK) && !m_strBuf.empty())
        {
            m_strBuf.pop_back();
            bChanged = true;
        }
        if (pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_RETURN)
         || pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_NUMPADENTER))
        {
            Commit();
            m_bFocused = false;
            ApplyFocusVisual();
            UpdateText();
            return;
        }
        // Mark "typing" so a click-away blur commits the name (mirrors numeric).
        if (bChanged) { m_bTyping = true; UpdateText(); }
    }

    void EditBox::PollKeyboard()
    {
        using namespace EditBox_detail;
        if (m_bTextMode) { PollText(); return; }
        auto* pInput = CInput::GetInst();
        if (!pInput) return;

        bool bChanged = false;
        auto beginType = [&] { if (!m_bTyping) { m_strBuf.clear(); m_bTyping = true; } };

        for (const KeyChar& kc : kDigits)
        {
            if (pInput->IsKey(CInput::KEY_STATE::DOWN, kc.dik))
            {
                beginType();
                if (m_strBuf.size() < kMaxTypeLen) { m_strBuf += kc.ch; bChanged = true; }
            }
        }

        // Decimal point (only when decimals are allowed and not already present).
        if ((pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_PERIOD)
          || pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_DECIMAL))
          && m_iDecimals > 0)
        {
            beginType();
            if (m_strBuf.find(L'.') == std::wstring::npos && m_strBuf.size() < kMaxTypeLen)
            {
                m_strBuf += L'.'; bChanged = true;
            }
        }

        // Sign (only for negative-allowed boxes) — toggles a leading '-'.
        if ((pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_MINUS)
          || pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_SUBTRACT))
          && m_bAllowNeg)
        {
            beginType();
            if (!m_strBuf.empty() && m_strBuf[0] == L'-') m_strBuf.erase(m_strBuf.begin());
            else                                          m_strBuf.insert(m_strBuf.begin(), L'-');
            bChanged = true;
        }

        // Backspace — edits from the currently shown string.
        if (pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_BACK))
        {
            m_bTyping = true;
            if (!m_strBuf.empty()) { m_strBuf.pop_back(); bChanged = true; }
        }

        // Enter commits and drops focus.
        if (pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_RETURN)
         || pInput->IsKey(CInput::KEY_STATE::DOWN, DIK_NUMPADENTER))
        {
            Commit();
            m_bFocused = false;
            ApplyFocusVisual();
            UpdateText();
            return;
        }

        if (bChanged)
        {
            // Parse the raw buffer (unclamped) so a live preview can update as
            // you type; clamping happens on commit (owner re-formats). ASCII.
            std::string s;
            for (wchar_t c : m_strBuf) s += static_cast<char>(c);
            m_fValue = static_cast<float>(std::atof(s.c_str()));
            UpdateText();
            if (m_fnOnChange) m_fnOnChange(m_fValue);
        }
    }

    void EditBox::Update(float fDeltaTime)
    {
        UIControl::Update(fDeltaTime);
        if (!m_bEnabled) return;

        auto* pInput = CInput::GetInst();
        if (!pInput) return;

        const float mx = static_cast<float>(pInput->GetMouseX());
        const float my = static_cast<float>(pInput->GetMouseY());
        const bool bInBox = mx >= m_fRect[0] && mx <= m_fRect[0] + m_fRect[2]
                         && my >= m_fRect[1] && my <= m_fRect[1] + m_fRect[3];

        // Click inside focuses; click anywhere else blurs. A blur after typing
        // commits the typed value.
        if (pInput->IsMouseButtonDown(CInput::MOUSE_TYPE::LEFT))
        {
            const bool bWasFocused = m_bFocused;
            m_bFocused = bInBox;
            if (bWasFocused && !m_bFocused && m_bTyping) Commit();
        }

        if (m_bFocused) PollKeyboard();

        if (m_bFocused != m_bFocusShown) { ApplyFocusVisual(); UpdateText(); }
    }

    std::shared_ptr<Component> EditBox::Clone()
    {
        return std::make_shared<EditBox>(*this);
    }
}

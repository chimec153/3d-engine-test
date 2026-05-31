#pragma once

#include "UIControl.h"
#include <functional>
#include <memory>
#include <string>

namespace Engine
{
    class Button;
    class Text;
    class Font;

    // A numeric edit box: a bordered panel you click to focus, then type into.
    // This engine has no WM_CHAR, so typing is read by polling DirectInput key
    // edges (CInput::IsKey) for digits / '.' / '-' / Backspace / Enter; the
    // relevant DIK keys are registered lazily in Init. Click inside to focus
    // (and start typing); click outside (anywhere else) to blur — focus is
    // self-managed from the box's own rect, so a sibling Slider needs no
    // coordination.
    //
    // Holds a float value. SetValue() is silent (formats the text, no
    // callback). SetOnChange fires per keystroke with the raw (unclamped)
    // parsed value (so a live preview can update while typing); SetOnCommit
    // fires on Enter or on blur-after-typing, when the value is "final" — the
    // owner clamps/formats by calling SetValue back.
    class ENGINE_DLL EditBox : public UIControl
    {
    public:
        EditBox();
        EditBox(const EditBox& other) = default;
        virtual ~EditBox() override = default;

        // Placement in screen pixels (own rect; not the base Transform).
        void SetBoxRect(float fX, float fY, float fW, float fH);
        void SetFont(const std::shared_ptr<Font>& pFont);
        void SetDecimals(int iDecimals) { m_iDecimals = iDecimals; }
        void SetAllowNegative(bool bAllow) { m_bAllowNeg = bAllow; }
        void SetValue(float fValue);     // silent: set + format the text
        float GetValue() const { return m_fValue; }
        void SetEnabled(bool bEnabled);
        bool IsFocused() const { return m_bFocused; }
        void SetOnChange(std::function<void(float)> fn) { m_fnOnChange = std::move(fn); }
        void SetOnCommit(std::function<void(float)> fn) { m_fnOnCommit = std::move(fn); }

        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    private:
        void Relayout();         // place the box + text from m_fRect
        void UpdateText();       // push the display string (+caret) to the Text
        void ApplyFocusVisual(); // swap the box texture on focus change
        void PollKeyboard();     // edit m_strBuf from DIK key edges
        void Commit();           // fire onCommit with the parsed value
        std::wstring Format(float fValue) const;

        std::shared_ptr<Button> m_pBox;
        std::shared_ptr<Text>   m_pText;

        float m_fRect[4]  = { 0.f, 0.f, 0.f, 0.f };

        float m_fValue    = 0.f;
        int   m_iDecimals = 1;
        bool  m_bAllowNeg = false;

        bool  m_bEnabled    = true;
        bool  m_bFocused    = false;
        bool  m_bFocusShown = false;   // box highlight state last applied
        bool  m_bTyping     = false;   // m_strBuf is a raw typed string, not Format()

        std::wstring m_strBuf;         // current edited text (formatted or typed)
        std::wstring m_strShown;       // last string pushed to m_pText (re-bake guard)

        std::function<void(float)> m_fnOnChange;
        std::function<void(float)> m_fnOnCommit;
    };
}

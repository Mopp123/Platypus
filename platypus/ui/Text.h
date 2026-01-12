#pragma once

#include "UIElement.h"
#include "LayoutUI.h"


namespace platypus
{
    namespace ui
    {
        // TODO: Make sure string is valid utf8!!!
        UIElement* add_text_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::string& text,
            const Vector4f& color,
            const Font* pFont
        );

        // NOTE: VERY inefficient!
        // DO NOT USE if editing some text, like input field
        //  -> those should modify just the parts that are
        //  actually modified!
        std::string wrap_text(
            const std::string& text,
            const Font* pFont,
            const UIElement* pParentElement,
            float& outMaxLineWidth,
            size_t& outLineCount
        );

        void set_text(
            UIElement* pTextElement,
            UIElement* pParentElement,
            const std::string& text
        );

        Vector2f get_char_scale(uint32_t codepoint, const Font* pFont);

        template <typename T>
        Vector2f get_text_scale(T text, const Font* pFont);
    }
}

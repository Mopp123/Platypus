#pragma once

#include "UIElement.hpp"
#include "LayoutUI.hpp"


namespace platypus
{
    namespace ui
    {
        // NOTE: Creates new layout for the text in which the scale gets modified to fit the
        // whole text inside it
        UIElement* add_text_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Vector4f& color,
            const Vector4f& hoverColor,
            const Vector4f& selectedColor,
            const std::string& text,
            const Font* pFont,
            uint32_t effectOnParentFlags = DEFAULT_EFFECT_ON_PARENT_FLAGS
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

        // Forces text to fit inside the parent.
        // If overflow happens, inserts "..." to indicate that.
        // Strips either the beginning or end of the inputted text.
        //  stripDirection == 0 : strips the end of the string
        //  stripDirection == 1 : strips the beginning of the string
        //
        // Header can be inserted to get result like:
        //  Current dir: .../Documents/someDir
        //
        // NOTE: Inefficient as fuck! +potentially dangerous:D
        //  -> Added to quickly make a filebrowser
        std::string strip_text_overflow_ellipsis(
            UIElement* pParentElement,
            const Font* pFont,
            const std::string& header,
            const std::string& text,
            TextOverflow overflow = TextOverflow::ELLIPSIS_RIGHT
        );

        Vector2f get_char_scale(uint32_t codepoint, const Font* pFont);

        template <typename T>
        Vector2f get_text_scale(T text, const Font* pFont);
    }
}

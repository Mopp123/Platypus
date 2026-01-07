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

        Vector2f get_text_scale(const std::string& text, const Font* pFont);
    }
}

#pragma once

#include "UIElement.h"
#include "LayoutUI.h"


namespace platypus
{
    namespace ui
    {
        UIElement* add_text_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::wstring& text,
            const Vector4f& color,
            const Font* pFont
        );

        std::wstring wrap_text(
            const std::wstring& text,
            const Font* pFont,
            const UIElement* pParentElement,
            float& outMaxLineWidth,
            size_t& outLineCount
        );

        void set_text(
            UIElement* pTextElement,
            UIElement* pParentElement,
            const std::wstring& text
        );

        Vector2f get_text_scale(const std::wstring& text, const Font* pFont);
    }
}

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

        Vector2f get_text_scale(const std::wstring& text, const Font* pFont);
    }
}

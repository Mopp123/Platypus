#pragma once

#include "Layout.hpp"
#include "platypus/assets/Font.hpp"

class UIManager;
namespace platypus
{
    namespace ui
    {
        void create_default_text_layout(
            UIManager& uiManager,
            uint32_t effectOnParentFlags,
            Layout** ppLayout
        );

        void create_default_button_layout(
            UIManager& uiManager,
            Layout** ppBoxLayout,
            Layout** ppTextLayout
        );

        void create_default_checkbox_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppLayout,
            Layout** ppBoxLayout
        );

        void create_default_input_field_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            ExpandElements fieldDirection, // is the field to the left or below the info txt
            Layout** ppRootLayout,
            Layout** ppFieldLayout
        );

        void create_default_input_field_cursor_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppLayout
        );
    }
}

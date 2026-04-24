#pragma once

#include "Layout.hpp"
#include "platypus/assets/Font.hpp"

class UIManager;
namespace platypus
{
    namespace ui
    {
        void create_default_text_layout(UIManager& uiManager, Layout** ppLayout);

        void create_default_button_layout(
            UIManager& uiManager,
            Layout** ppBoxLayout,
            Layout** ppTextLayout
        );

        void create_default_checkbox_layout(UIManager& uiManager, Layout** ppLayout);

        void create_default_input_field_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppRootLayout,
            Layout** ppFieldLayout,
            Layout** ppCursorLayout
        );
    }
}

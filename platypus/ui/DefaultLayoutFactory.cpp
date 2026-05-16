#include "DefaultLayoutFactory.hpp"
#include "UIManager.hpp"


namespace platypus
{
    namespace ui
    {
        void create_default_text_layout(
            UIManager& uiManager,
            uint32_t effectOnParentFlags,
            Layout** ppLayout
        )
        {
            Layout* pLayout = uiManager.createLayout();
            pLayout->effectOnParentFlags = effectOnParentFlags;
            pLayout->colors = {
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f }
            };
            *ppLayout = pLayout;
        }


        void create_default_button_layout(
            UIManager& uiManager,
            Layout** ppBoxLayout,
            Layout** ppTextLayout
        )
        {
            Layout* pBoxLayout = uiManager.createLayout();
            pBoxLayout->colors = {
                { 0.175f, 0.125f, 0.125f, 1.0f },
                { 0.3f, 0.3f, 0.3f, 1.0f },
                { 0.05f, 0.05f, 0.05f, 1.0f },
                { 0.2f, 0.2f, 0.2f, 1.0f }
            };
            pBoxLayout->padding = { 2, 2 };
            pBoxLayout->borderThickness = 1.0f;
            pBoxLayout->horizontalContentAlignment = ui::HorizontalAlignment::CENTER;
            pBoxLayout->verticalContentAlignment = ui::VerticalAlignment::CENTER;

            Layout* pTextLayout = uiManager.createLayout();
            pTextLayout->colors = {
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f }
            };

            *ppBoxLayout = pBoxLayout;
            *ppTextLayout = pTextLayout;
        }


        void create_default_checkbox_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppLayout,
            Layout** ppBoxLayout
        )
        {
            Layout* pLayout = uiManager.createLayout();
            pLayout->expandElements = ExpandElements::RIGHT;

            Layout* pBoxLayout = uiManager.createLayout();
            pBoxLayout->colors = {
                { 0.2f, 0.2f, 0.2f, 1.0f },
                { 0.3f, 0.3f, 0.3f, 1.0f },
                { 0.175f, 0.175f, 0.175f, 1.0f },
                { 0.125f, 0.125f, 0.125f, 1.0f }
            };
            Vector2f defaultBoxTextScale = get_text_scale(std::string("X"), pDefaultFont);
            float useScale = std::max(defaultBoxTextScale.x, defaultBoxTextScale.y);
            pBoxLayout->scale = { useScale, useScale };
            pBoxLayout->horizontalContentAlignment = HorizontalAlignment::CENTER;
            pBoxLayout->verticalContentAlignment = VerticalAlignment::CENTER;
            pBoxLayout->padding = { 1, 1 };
            pBoxLayout->borderThickness = 1.0f;

            *ppLayout = pLayout;
            *ppBoxLayout = pBoxLayout;
        }


        void create_default_input_field_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            ExpandElements fieldDirection, // is the field to the left or below the info txt
            Layout** ppRootLayout,
            Layout** ppFieldLayout
        )
        {
            Layout* pRootLayout = uiManager.createLayout();
            pRootLayout->expandElements = fieldDirection;

            float charHeight = static_cast<float>(pDefaultFont->getFittingHeight());

            Layout* pFieldLayout = uiManager.createLayout();
            pFieldLayout->expandElements = ExpandElements::RIGHT;

            // NOTE: Atm usign 300px
            // *Should scale be overridden in order to use same layout for differently scaled
            // InputFields?
            pFieldLayout->scale = { 300, charHeight };
            pFieldLayout->colors.base = { 0.1f, 0.1f, 0.1f, 1.0f };
            pFieldLayout->colors.hover = { 0.3f, 0.3f, 0.3f, 1.0f };
            pFieldLayout->colors.selected = { 0.125f, 0.125f, 0.125f, 1.0f };
            pFieldLayout->colors.border = { 0.5f, 0.5f, 0.5f, 1.0f };
            pFieldLayout->borderThickness = 1.0f;
            pFieldLayout->textOverflow = TextOverflow::ELLIPSIS_LEFT;

            *ppRootLayout = pRootLayout;
            *ppFieldLayout = pFieldLayout;
        }

        void create_default_input_field_cursor_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppLayout
        )
        {
            Layout* pLayout = uiManager.createLayout();
            pLayout->colors = {
                { 1, 1, 1, 1 },
                { 1, 1, 1, 1 },
                { 1, 1, 1, 1 }
            };
            pLayout->scale = { 2.0f, static_cast<float>(pDefaultFont->getFittingHeight()) };
            pLayout->effectOnParentFlags = ui::EffectOnParentFlagBits::INCREMENT_POSITION;
            *ppLayout = pLayout;
        }
    }
}

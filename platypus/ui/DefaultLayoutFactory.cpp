#include "DefaultLayoutFactory.hpp"
#include "UIManager.hpp"


namespace platypus
{
    namespace ui
    {
        void create_default_text_layout(UIManager& uiManager, Layout** ppLayout)
        {
            Layout* pLayout = uiManager.createLayout();
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
                { 0.2f, 0.2f, 0.2f, 1.0f },
                { 0.3f, 0.3f, 0.3f, 1.0f },
                { 0.1f, 0.1f, 0.1f, 1.0f },
                { 0.2f, 0.2f, 0.2f, 1.0f }
            };
            pBoxLayout->padding = { 2, 2 };
            pBoxLayout->borderThickness = 1.0f;

            Layout* pTextLayout = uiManager.createLayout();
            pTextLayout->colors = {
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f }
            };

            *ppBoxLayout = pBoxLayout;
            *ppTextLayout = pTextLayout;
        }


        void create_default_checkbox_layout(UIManager& uiManager, Layout** ppLayout)
        {
            Layout* pLayout = uiManager.createLayout();
            pLayout->expandElements = ExpandElements::RIGHT;
            *ppLayout = pLayout;
        }


        void create_default_input_field_layout(
            UIManager& uiManager,
            Font* pDefaultFont,
            Layout** ppRootLayout,
            Layout** ppFieldLayout,
            Layout** ppCursorLayout
        )
        {
            Layout* pRootLayout = uiManager.createLayout();

            Layout* pFieldLayout = uiManager.createLayout();
            pFieldLayout->expandElements = ExpandElements::RIGHT;
            pFieldLayout->scale = { }; // NOTE: This might cause issues!
            pFieldLayout->colors.base = { 0.2f, 0.2f, 0.2f, 1.0f };
            pFieldLayout->colors.hover = { 0.3f, 0.3f, 0.3f, 1.0f };
            pFieldLayout->colors.selected = { 0.1f, 0.1f, 0.1f, 1.0f };
            pFieldLayout->colors.border = { 0.7f, 0.7f, 0.7f, 1.0f };
            pFieldLayout->borderThickness = 1.0f;

            Layout* pCursorLayout = uiManager.createLayout();
            pCursorLayout->colors = {
                { 1, 1, 1, 1 },
                { 1, 1, 1, 1 },
                { 1, 1, 1, 1 }
            };
            // NOTE: Atm usign 300px
            // *Should scale be overridden in order to use same layout for differently scaled
            // InputFields?
            pCursorLayout->scale = { 300.0f, static_cast<float>(pDefaultFont->getFittingHeight()) };
            pCursorLayout->effectOnParentFlags = ui::EffectOnParentFlagBits::INCREMENT_POSITION;

            *ppRootLayout = pRootLayout;
            *ppFieldLayout = pFieldLayout;
            *ppCursorLayout = pCursorLayout;
        }
    }
}

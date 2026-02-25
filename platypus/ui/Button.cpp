#include "Button.hpp"
#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/core/Scene.hpp"


namespace platypus
{
    namespace ui
    {
        void ButtonMouseEnterEvent::func(int mx, int my)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                _button.pBox->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _button.pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            if (pBoxRenderable)
                pBoxRenderable->color = _button.highlightColor;
            if (pTextRenderable)
                pTextRenderable->color = _button.textHighlightColor;
        }

        void ButtonMouseExitEvent::func(int mx, int my)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                _button.pBox->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _button.pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            if (pBoxRenderable)
                pBoxRenderable->color = _button.originalColor;
            if (pTextRenderable)
                pTextRenderable->color = _button.originalTextColor;
        }


        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::string& text,
            const Font* pFont
        )
        {
            Vector4f originalColor = LayoutUI::s_config.buttonColor;
            Vector4f highlightColor = LayoutUI::s_config.buttonHighlightColor;
            Vector4f originalTextColor = LayoutUI::s_config.buttonTextColor;
            Vector4f textHighlightColor = LayoutUI::s_config.buttonTextHighlightColor;

            float padding = 2.0f;
            Vector2f textScale = get_text_scale(text, pFont);
            textScale.x += padding;
            textScale.y += padding;

            Layout layout {
                { 0, 0 }, // pos
                textScale, // scale
                originalColor // color
            };

            UIElement* pContainer = add_container(ui, pParent, layout, true);
            UIElement* pText = add_text_element(
                ui,
                pContainer,
                text,
                originalTextColor,
                pFont
            );

            Button button =
            {
                originalColor,
                highlightColor,
                originalTextColor,
                textHighlightColor,
                pContainer,
                pText
            };

            pContainer->_pMouseEnterEvent = new ButtonMouseEnterEvent(button);
            pContainer->_pMouseExitEvent = new ButtonMouseExitEvent(button);

            return pContainer;
        }

        Button add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            const Vector4f& highlightColor,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            Layout useLayout = layout;

            Vector2f textScale = get_text_scale(text, pFont);

            if (useLayout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_HORIZONTALLY)
                useLayout.scale.x = textScale.x + useLayout.padding.x * 2.0f;

            if (useLayout.stretchFitContentFlags & StretchFitContentFlagBits::STRETCH_FIT_CONTENT_VERTICALLY)
                useLayout.scale.y = textScale.y + useLayout.padding.y * 2.0f;

            UIElement* pContainer = add_container(
                ui,
                pParent,
                useLayout,
                true, // create renderable
                NULL_ID, // textureID
                nullptr, // pFont
                pOnClick
            );

            UIElement* pText = add_text_element(
                ui,
                pContainer,
                text,
                textColor,
                pFont
            );

            Button button = {
                layout.color,
                highlightColor,
                textColor,
                textHighlightColor,
                pContainer,
                pText
            };

            if (pOnEnter)
                pContainer->_pMouseEnterEvent = pOnEnter;
            else
                pContainer->_pMouseEnterEvent = new ButtonMouseEnterEvent(button);

            if (pOnExit)
                pContainer->_pMouseExitEvent = pOnExit;
            else
                pContainer->_pMouseExitEvent = new ButtonMouseExitEvent(button);

            return button;
        }


        void reset_button(const Button& button)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            #ifdef PLATYPUS_DEBUG
            if (!button.pBox)
            {
                Debug::log(
                    "No button's box element was nullptr",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            if (!button.pText)
            {
                Debug::log(
                    "No button's text element was nullptr",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            GUIRenderable* pBoxRenderable = button.pBox->getRenderable();
            GUIRenderable* pTextRenderable = button.pText->getRenderable();
            pBoxRenderable->color = button.originalColor;
            pTextRenderable->color = button.originalTextColor;
        }
    }
}

#include "Button.h"
#include "Text.h"
#include "platypus/core/Application.h"


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
            const std::wstring& text,
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
    }
}

#include "Button.hpp"
#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"


namespace platypus
{
    namespace ui
    {
        void ButtonMouseEnterEvent::func(int mx, int my)
        {
            if (_button.pBox->isSelected())
                return;

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
            if (_button.pBox->isSelected())
                return;

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

        ButtonSelectEvent::ButtonSelectEvent(
            Scene* pScene,
            Button button,
            UIElement::OnClickEvent* pUserEvent
        ) :
            _pScene(pScene),
            _button(button)
        {
        }

        ButtonSelectEvent::~ButtonSelectEvent()
        {
        }

        // TODO: Allow modifying border color
        void ButtonSelectEvent::func(MouseButtonName button, InputAction action)
        {
            GUIRenderable* pBoxRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(_button.pBox->getEntityID(), ComponentType::COMPONENT_TYPE_GUI_RENDERABLE)
            );
            GUIRenderable* pTextRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(_button.pText->getEntityID(), ComponentType::COMPONENT_TYPE_GUI_RENDERABLE)
            );
            pBoxRenderable->color = _button.selectedColor;
            pTextRenderable->color = _button.textSelectedColor;

            //if (_pUserEvent)
            //    _pUserEvent->func(button, action);
        }


        ButtonDeselectEvent::ButtonDeselectEvent(
            Scene* pScene,
            Button button,
            UIElement::OnClickEvent* pUserEvent
        ) :
            _pScene(pScene),
            _button(button)
        {
        }

        // TODO: Allow modifying border color
        void ButtonDeselectEvent::func(MouseButtonName button, InputAction action)
        {
            GUIRenderable* pBoxRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(_button.pBox->getEntityID(), ComponentType::COMPONENT_TYPE_GUI_RENDERABLE)
            );
            GUIRenderable* pTextRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(_button.pText->getEntityID(), ComponentType::COMPONENT_TYPE_GUI_RENDERABLE)
            );
            pBoxRenderable->color = _button.originalColor;
            pTextRenderable->color = _button.originalTextColor;

            //if (_pUserEvent)
            //    _pUserEvent->func(button, action);
        }

        ButtonDeselectEvent::~ButtonDeselectEvent()
        {
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
                { 0, 0, 0, 0 },
                originalTextColor,
                textHighlightColor,
                { 0, 0, 0, 0 },
                pContainer,
                pText
            };

            pContainer->_pMouseEnterEvent = new ButtonMouseEnterEvent(button);
            pContainer->_pMouseExitEvent = new ButtonMouseExitEvent(button);

            return pContainer;
        }

        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            const Vector4f& highlightColor,
            const Vector4f& selectedColor,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const Vector4f& textSelectedColor,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            bool selectable,
            ButtonSelectEvent* pOnSelect,
            ButtonDeselectEvent* pOnDeselect
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
                pOnClick,
                selectable,
                nullptr,
                nullptr
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
                selectedColor,
                textColor,
                textHighlightColor,
                textSelectedColor,
                pContainer,
                pText
            };

            if (selectable)
            {
                Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
                pContainer->_pOnSelectEvent = new ButtonSelectEvent(pScene, button, nullptr);
                pContainer->_pOnDeselectEvent = new ButtonDeselectEvent(pScene, button, nullptr);
            }
            pContainer->_pMouseEnterEvent = new ButtonMouseEnterEvent(button);
            pContainer->_pMouseExitEvent = new ButtonMouseExitEvent(button);

            return pContainer;
        }
    }
}

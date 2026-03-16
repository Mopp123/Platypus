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
                _buttonRef.pBox->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef.pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );

            if (pBoxRenderable)
                pBoxRenderable->color = _buttonRef.pBox->getLayout().hoverColor;
            if (pTextRenderable)
                pTextRenderable->color = _buttonRef.pText->getLayout().hoverColor;
        }

        void ButtonMouseExitEvent::func(int mx, int my)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef.pBox->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef.pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            if (pBoxRenderable)
                pBoxRenderable->color = _buttonRef.pBox->getLayout().color;
            if (pTextRenderable)
                pTextRenderable->color = _buttonRef.pText->getLayout().color;
        }


        Button add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& boxLayout,
            const Layout& textLayout,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            UIElement* pContainer = add_container(
                ui,
                pParent,
                boxLayout,
                true, // create renderable
                NULL_ID, // textureID
                nullptr, // pFont
                pOnClick
            );

            UIElement* pText = add_text_element(
                ui,
                pContainer,
                textLayout,
                text,
                pFont
            );

            Button button = {
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
            #ifdef PLATYPUS_DEBUG
            if (!button.pBox)
            {
                Debug::log(
                    "Button's box element was nullptr",
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
            pBoxRenderable->color = button.pBox->getLayout().color;
            pTextRenderable->color = button.pText->getLayout().color;
        }
    }
}

#include "Button.hpp"
#include "UIManager.hpp"
#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/core/Scene.hpp"


namespace platypus
{
    namespace ui
    {
        void Button::MouseEnterEvent::func(int mx, int my)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef.getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef._pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );

            if (pBoxRenderable)
                pBoxRenderable->color = _buttonRef.getLayout()->hoverColor;
            if (pTextRenderable)
                pTextRenderable->color = _buttonRef._pText->getLayout()->hoverColor;
        }

        void Button::MouseExitEvent::func(int mx, int my)
        {
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef.getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                _buttonRef._pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );

            if (_buttonRef.isSelected())
            {
                pBoxRenderable->color = _buttonRef.getLayout()->selectedColor;
                pTextRenderable->color = _buttonRef._pText->getLayout()->selectedColor;
            }
            else
            {
                pBoxRenderable->color = _buttonRef.getLayout()->color;
                pTextRenderable->color = _buttonRef._pText->getLayout()->color;
            }
        }

        Button::Button(
            UIManager& uiManager,
            const Layout* pLayout,
            const Vector4f& textColor,
            const Vector4f& textHoverColor,
            const Vector4f& textSelectedColor,
            uint32_t textEffectOnParentFlags,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        ) :
            UIElement(
                uiManager,
                pLayout,
                true,
                NULL_ID,
                pOnClick
            )
        {
            _pText = uiManager.createText(
                this,
                pFont,
                textColor,
                textHoverColor,
                textSelectedColor,
                text,
                textEffectOnParentFlags
            );

            if (pOnEnter)
                _pMouseEnterEvent = pOnEnter;
            else
                _pMouseEnterEvent = new Button::MouseEnterEvent(*this);

            if (pOnExit)
                _pMouseExitEvent = pOnExit;
            else
                _pMouseExitEvent = new Button::MouseExitEvent(*this);
        }

        // Currently almost the same as UIElement's setActive but this also resets
        // the button's colors... dumb...
        void Button::setActive(bool arg)
        {
            reset();
            if (!arg)
                remove_from_cursor_over_layers(getAbsoluteLayer(), _entityID);

            // *if setting inactive by OnClick func, reset mouseOver
            // NOTE: This might be an issue if setting active and cursor immediately over?
            _isCursorOver = false;
            _dragged = false;
            for (UIElement* pChild : _children)
                pChild->setActive(arg);

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            pScene->setEntityActive(_entityID, arg);
        }

        void Button::reset()
        {
            #ifdef PLATYPUS_DEBUG
            if (!_pText)
            {
                Debug::log(
                    "Button's text element was nullptr",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            GUIRenderable* pBoxRenderable = getRenderable();
            GUIRenderable* pTextRenderable = _pText->getRenderable();
            pBoxRenderable->color = getLayout()->color;
            pTextRenderable->color = _pText->getLayout()->color;
        }
        /*
        Button add_button_element(
            UIManager& uiManager,
            UIElement* pParent,
            const Layout* pBoxLayout,
            const Vector4f& textColor,
            const Vector4f& textHoverColor,
            const Vector4f& textSelectedColor,
            uint32_t textEffectOnParentFlags,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter,
            UIElement::MouseExitEvent* pOnExit
        )
        {
            UIElement* pContainer = uiManager.createElement(
                pParent,
                pBoxLayout,
                true, // create renderable
                NULL_ID, // textureID
                pOnClick
            );

            Text* pText = uiManager.createText(
                pContainer,
                pFont,
                textColor,
                textHoverColor,
                textSelectedColor,
                text,
                textEffectOnParentFlags
            );

            Button button = {
                pContainer,
                pText
            };

            if (pOnEnter)
                pContainer->_pMouseEnterEvent = pOnEnter;
            else
                pContainer->_pMouseEnterEvent = new ButtonMouseEnterEvent(pContainer, pText);

            if (pOnExit)
                pContainer->_pMouseExitEvent = pOnExit;
            else
                pContainer->_pMouseExitEvent = new ButtonMouseExitEvent(pContainer, pText);

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
            pBoxRenderable->color = button.pBox->getLayout()->color;
            pTextRenderable->color = button.pText->getLayout()->color;
        }
        */
    }
}

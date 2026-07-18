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
        void on_mouse_enter_button_default(int mx, int my, void* pUserData)
        {
            Button* pButton = reinterpret_cast<Button*>(pUserData);
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                pButton->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                pButton->_pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );

            if (pBoxRenderable)
                pBoxRenderable->color = pButton->getLayout()->colors.hover;
            if (pTextRenderable)
                pTextRenderable->color = pButton->_pText->getLayout()->colors.hover;
        }

        void on_mouse_exit_button_default(int mx, int my, void* pUserData)
        {
            Button* pButton = reinterpret_cast<Button*>(pUserData);
            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            GUIRenderable* pBoxRenderable = (GUIRenderable*)pScene->getComponent(
                pButton->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );
            GUIRenderable* pTextRenderable = (GUIRenderable*)pScene->getComponent(
                pButton->_pText->getEntityID(),
                ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
            );

            if (pButton->isSelected())
            {
                pBoxRenderable->color = pButton->getLayout()->colors.selected;
                pTextRenderable->color = pButton->_pText->getLayout()->colors.selected;
            }
            else
            {
                pBoxRenderable->color = pButton->getLayout()->colors.base;
                pTextRenderable->color = pButton->_pText->getLayout()->colors.base;
            }
        }

        Button::Button(
            UIManager& uiManager,
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const std::string& text,
            const Font* pFont,
            void(*pOnClick)(MouseButtonName, InputAction, void*),
            void* pOnClickUserData,
            void(*pOnMouseEnter)(int mx, int my, void* pUserData),
            void* pOnMouseEnterUserData,
            void(*pOnMouseExit)(int mx, int my, void* pUserData),
            void* pOnMouseExitUserData
        ) :
            UIElement(
                uiManager,
                pParent,
                pLayout,
                true,
                NULL_UUID,
                nullptr,
                pOnClick,
                pOnClickUserData
            )
        {
            _pText = uiManager.createText(
                this,
                pTextLayout,
                text,
                pFont
            );

            if (pOnMouseEnter)
            {
                _pOnMouseEnter = pOnMouseEnter;
                _pOnMouseEnterUserData = pOnMouseEnterUserData;
            }
            else
            {
                _pOnMouseEnter = &on_mouse_enter_button_default;
                _pOnMouseEnterUserData = this;
            }

            if (pOnMouseExit)
            {
                _pOnMouseExit = pOnMouseExit;
                _pOnMouseExitUserData = pOnMouseExitUserData;
            }
            else
            {
                _pOnMouseExit = &on_mouse_exit_button_default;
                _pOnMouseExitUserData = this;
            }

            // Need to update full tree since after adding the
            // text element, the scale changes
            triggerFullTreeUpdate();
        }

        // Currently almost the same as UIElement's setActive but this also resets
        // the button's colors... dumb...
        void Button::setActive(bool arg)
        {
            if (!arg)
            {
                // NOTE: resetting the button's color to base kind of fucks up some stuff...
                //reset();
                remove_from_cursor_over_layers(getAbsoluteLayer(), _entityID);
            }

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
            pBoxRenderable->color = getLayout()->colors.base;
            pTextRenderable->color = _pText->getLayout()->colors.base;
        }
    }
}

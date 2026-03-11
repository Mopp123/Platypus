#include "InputField.hpp"
#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/utils/StringUtils.hpp"


namespace platypus
{
    namespace ui
    {
        void InputFieldMouseEnterEvent::func(int mx, int my)
        {
            const Button& buttonElement = _inputField.button;
            GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(
                    buttonElement.pBox->getEntityID(),
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pButtonRenderable && !_inputField.pContainer->isSelected())
                pButtonRenderable->color = buttonElement.highlightColor;
        }


        void InputFieldMouseExitEvent::func(int mx, int my)
        {
            const Button& buttonElement = _inputField.button;
            GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(
                    buttonElement.pBox->getEntityID(),
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pButtonRenderable && !_inputField.pContainer->isSelected())
            {
                pButtonRenderable->color = buttonElement.originalColor;
            }
        }


        void InputFieldMouseButtonEvent::func(MouseButtonName button, InputAction action, int mods)
        {
            if (button == MouseButtonName::MOUSE_LEFT && action == InputAction::PRESS)
            {
                const Button& buttonElement = _inputField.button;
                GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                    _pScene->getComponent(
                        buttonElement.pBox->getEntityID(),
                        ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                    )
                );
                UIElement* pContainer = _inputField.pContainer;
                bool wasSelected = pContainer->isSelected();
                if (_inputField.button.pBox->isCursorOver())
                {
                    if (!wasSelected)
                        _inputField.pContainer->setSelected(true);
                    else
                        _inputField.pContainer->setSelected(false);

                }
                else
                {
                    _inputField.pContainer->setSelected(false);
                }

                if (pContainer->isSelected())
                {
                    pButtonRenderable->color = _inputField.activeColor;

                    // Add "|" at to the input field to indicate activeness...
                    UIElement* pTextElement = _inputField.button.pText;
                    std::string str = pTextElement->getRenderable()->text + "|";
                    set_text(
                        pTextElement,
                        _inputField.button.pBox,
                        str
                    );
                }
                else
                {
                    if (wasSelected)
                    {
                        pButtonRenderable->color = buttonElement.originalColor;

                        // Erase the "|"
                        UIElement* pTextElement = _inputField.button.pText;
                        std::string& str = pTextElement->getRenderable()->text;
                        util::str::pop_back_utf8(str);
                        set_text(
                            pTextElement,
                            _inputField.button.pBox,
                            str
                        );
                    }
                }
            }
        }


        void InputFieldCharInputEvent::func(unsigned int codepoint)
        {
            if (_inputField.pContainer->isSelected())
            {
                UIElement* pTextElement = _inputField.button.pText;
                std::string& str = pTextElement->getRenderable()->text;

                // First erase the previously added "|"
                util::str::pop_back_utf8(str);

                // Then add the actual char
                util::str::append_utf8(codepoint, str);

                // Then add the "|" back
                str += "|";

                set_text(
                    pTextElement,
                    _inputField.button.pBox,
                    str
                );
            }
        }


        void InputFieldKeyEvent::func(KeyName key, int scancode, InputAction action, int mods)
        {
            if (_inputField.pContainer->isSelected())
            {
                UIElement* pTextElement = _inputField.button.pText;
                std::string& str = pTextElement->getRenderable()->text;
                if (key == KeyName::KEY_BACKSPACE && action != InputAction::RELEASE)
                {
                    // first erase the "|"
                    util::str::pop_back_utf8(str);

                    // Then the actual char
                    util::str::pop_back_utf8(str);

                    // Then put back the "|"
                    str += "|";

                    set_text(
                        pTextElement,
                        _inputField.button.pBox,
                        str
                    );
                }
                else if (key == KeyName::KEY_ENTER)
                {
                    // Erase the "|"
                    util::str::pop_back_utf8(str);
                    set_text(
                        pTextElement,
                        _inputField.button.pBox,
                        str
                    );

                    _inputField.pContainer->setSelected(false);

                    const Button& buttonElement = _inputField.button;
                    GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                        _pScene->getComponent(
                            buttonElement.pBox->getEntityID(),
                            ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                        )
                    );
                    pButtonRenderable->color = buttonElement.originalColor;
                }
            }
        }


        InputField add_input_field_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            TextOverflow overflow,
            const Vector4f& highlightColor,
            const Vector4f& activeColor,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const std::string& infoText,
            const Font* pFont
        )
        {
            UIElement* pRootContainer = add_container(
                ui,
                pParent,
                layout,
                false
            );

            Layout infoTextLayout;
            infoTextLayout.color = textColor;
            UIElement* pInfoTextElement = add_text_element(
                ui,
                pRootContainer,
                infoTextLayout,
                infoText,
                pFont
            );

            if (layout.scale.x == 0.0f || layout.scale.y == 0.0f)
            {
                Debug::log(
                    "Inputted layout's scale was: " + layout.scale.toString() + " "
                    "InputFields created via this function don't scale with "
                    "content text element (for testing ellipsis overflow) so you'll "
                    "need to provide the full scale if the input field in advance!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            Layout buttonLayout;
            buttonLayout.textOverflow = overflow;
            buttonLayout.scale = layout.scale;
            buttonLayout.color = layout.color;
            buttonLayout.padding = { 0, 0 };
            buttonLayout.effectOnParentFlags = layout.effectOnParentFlags;
            buttonLayout.borderColor = layout.borderColor;
            buttonLayout.borderThickness = layout.borderThickness;

            Layout textLayout;
            textLayout.color = textColor;
            textLayout.effectOnParentFlags = 0;

            Button buttonElement = add_button_element(
                ui,
                pRootContainer,
                buttonLayout,
                textLayout,
                highlightColor,
                textHighlightColor,
                "",
                pFont,
                nullptr, //UIElement::OnClickEvent* pOnClick,
                nullptr, //UIElement::MouseEnterEvent* pOnEnter,
                nullptr //UIElement::MouseExitEvent* pOnExit
            );

            InputField inputField = {
                activeColor,
                buttonElement,
                pRootContainer,
                pInfoTextElement
            };

            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();

            // Disgusting but need to do this this way for now...
            buttonElement.pBox->_pMouseEnterEvent = new InputFieldMouseEnterEvent(pScene, inputField);
            buttonElement.pBox->_pMouseExitEvent = new InputFieldMouseExitEvent(pScene, inputField);

            InputManager& inputManager = pApp->getInputManager();
            inputManager.addMouseButtonEvent(new InputFieldMouseButtonEvent(pScene, inputField));
            inputManager.addCharInputEvent(new InputFieldCharInputEvent(inputField));
            inputManager.addKeyEvent(new InputFieldKeyEvent(pScene, inputField));

            return inputField;
        }


        std::string get_input_field_content(InputField inputField)
        {
            #ifdef PLATYPUS_DEBUG
            if (!inputField.button.pText)
            {
                Debug::log(
                    "Input field's text element was nullptr!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
            if (!inputField.button.pText->getRenderable())
            {
                Debug::log(
                    "Input field's text element's renderable component was nullptr!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
            #endif
            return inputField.button.pText->getRenderable()->text;
        }
    }
}

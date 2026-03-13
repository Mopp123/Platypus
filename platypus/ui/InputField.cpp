#include "InputField.hpp"
#include "Text.hpp"
#include "platypus/core/Application.hpp"
#include "platypus/core/Debug.hpp"
#include "platypus/utils/StringUtils.hpp"


namespace platypus
{
    namespace ui
    {
        static void set_input_mode(const InputField& inputField, bool active)
        {
            UIElement* pCursorIndicator = inputField.pCursorIndicator;
            pCursorIndicator->setActive(active);
            const Button& button = inputField.button;
            GUIRenderable* pButtonBoxRenderable = button.pBox->getRenderable();
            if (active)
            {
                Vector2f cursorScale = pCursorIndicator->getLayout().scale;
                // TODO: When adding functionality to select existing chars in the string
                //  -> make the cursor be the scale of the selected char!
                const float cursorWidth = 2;
                pCursorIndicator->setLayoutScale({ cursorWidth, cursorScale.y });

                pButtonBoxRenderable->color = inputField.activeColor;
            }
            else
            {
                pButtonBoxRenderable->color = button.originalColor;
            }
        }

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
                    set_input_mode(_inputField, true);
                else if (wasSelected)
                    set_input_mode(_inputField, false);
            }
        }


        void InputFieldCharInputEvent::func(unsigned int codepoint)
        {
            if (_inputField.pContainer->isSelected())
            {
                UIElement* pTextElement = _inputField.button.pText;
                std::string& str = pTextElement->getRenderable()->text;
                util::str::append_utf8(codepoint, str);
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
                    // Then the actual char
                    util::str::pop_back_utf8(str);
                    set_text(
                        pTextElement,
                        _inputField.button.pBox,
                        str
                    );
                }
                else if (key == KeyName::KEY_ENTER)
                {
                    _inputField.pContainer->setSelected(false);
                    set_input_mode(_inputField, false);
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
            buttonLayout.expandElements = ExpandElements::RIGHT;

            Layout textLayout;
            textLayout.color = textColor;
            // NOTE: textLayout.effectOnParentFlags was 0 earlier for some reason...
            // TODO: Maybe allow defining the text layout separately for the InputField?
            textLayout.effectOnParentFlags = EffectOnParentFlagBits::STRETCH_VERTICALLY |
                EffectOnParentFlagBits::INCREMENT_POSITION;

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

            // TODO: When adding the functionality to move the cursor behind the
            // end of the string -> enable more control over the cursor color
            Layout cursorIndicatorLayout;
            cursorIndicatorLayout.color = textLayout.color;
            cursorIndicatorLayout.scale = { 0, static_cast<float>(pFont->getFittingHeight()) };
            cursorIndicatorLayout.effectOnParentFlags = EffectOnParentFlagBits::INCREMENT_POSITION;
            UIElement* pCursorIndicator = add_container(
                ui,
                buttonElement.pBox,
                cursorIndicatorLayout,
                true
            );
            pCursorIndicator->setActive(false);

            InputField inputField = {
                activeColor,
                buttonElement,
                pRootContainer,
                pInfoTextElement,
                pCursorIndicator
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

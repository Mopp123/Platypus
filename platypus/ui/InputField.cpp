#include "InputField.hpp"
#include "UIManager.hpp"
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
            Button* pButton = inputField.pButton;
            GUIRenderable* pButtonBoxRenderable = pButton->getRenderable();
            const Layout* pBoxLayout = pButton->getLayout();
            if (active)
            {
                Vector2f cursorScale = pCursorIndicator->getLayout()->scale;
                // TODO: When adding functionality to select existing chars in the string
                //  -> make the cursor be the scale of the selected char!
                const float cursorWidth = 2;
                pCursorIndicator->setLayoutScale({ cursorWidth, cursorScale.y });
                pButtonBoxRenderable->color = pBoxLayout->selectedColor;
            }
            else
            {
                pButtonBoxRenderable->color = pBoxLayout->color;
            }
        }

        void InputFieldMouseEnterEvent::func(int mx, int my)
        {
            Button* pButton = _inputField.pButton;
            GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(
                    pButton->getEntityID(),
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pButtonRenderable && !_inputField.pContainer->isSelected())
                pButtonRenderable->color = pButton->getLayout()->hoverColor;
        }


        void InputFieldMouseExitEvent::func(int mx, int my)
        {
            Button* pButton = _inputField.pButton;
            GUIRenderable* pButtonRenderable = reinterpret_cast<GUIRenderable*>(
                _pScene->getComponent(
                    pButton->getEntityID(),
                    ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
                )
            );
            if (pButtonRenderable && !_inputField.pContainer->isSelected())
                pButtonRenderable->color = pButton->getLayout()->color;
        }


        void InputFieldMouseButtonEvent::func(MouseButtonName button, InputAction action, int mods)
        {
            if (button == MouseButtonName::MOUSE_LEFT && action == InputAction::PRESS)
            {
                UIElement* pContainer = _inputField.pContainer;
                bool wasSelected = pContainer->isSelected();
                if (_inputField.pButton->isCursorOver())
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
                Text* pText = _inputField.pButton->getText();
                std::string& str = pText->getRenderable()->text;
                util::str::append_utf8(codepoint, str);
                pText->set(
                    _inputField.pButton,
                    str
                );
            }
        }


        void InputFieldKeyEvent::func(KeyName key, int scancode, InputAction action, int mods)
        {
            if (_inputField.pContainer->isSelected())
            {
                Text* pText = _inputField.pButton->getText();
                std::string& str = pText->getRenderable()->text;
                if (key == KeyName::KEY_BACKSPACE && action != InputAction::RELEASE)
                {
                    // Then the actual char
                    util::str::pop_back_utf8(str);
                    pText->set(
                        _inputField.pButton,
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
            UIManager& uiManager,
            UIElement* pParent,
            const Layout* pLayout,
            TextOverflow overflow,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const std::string& infoText,
            const Font* pFont
        )
        {
            UIElement* pRootContainer = uiManager.createElement(
                pParent,
                pLayout,
                false
            );

            Text* pInfoTextElement = uiManager.createText(
                pRootContainer,
                pFont,
                textColor,
                textColor, // hover color
                textColor, // selected color
                infoText
            );

            if (pLayout->scale.x == 0.0f || pLayout->scale.y == 0.0f)
            {
                Debug::log(
                    "Inputted layout's scale was: " + pLayout->scale.toString() + " "
                    "InputFields created via this function don't scale with "
                    "content text element (for testing ellipsis overflow) so you'll "
                    "need to provide the full scale if the input field in advance!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            Layout* pButtonLayout = uiManager.createLayout();
            pButtonLayout->textOverflow = overflow;

            // Decrement the info width padding and elem gap from the button's width
            //  -> otherwise its' scale is incorrect in relation to the "root element"
            float infoWidth = get_text_scale(infoText, pFont).x;
            pButtonLayout->scale = {
                pLayout->scale.x - (infoWidth + pLayout->padding.x * 2.0f + pLayout->elementGap),
                pLayout->scale.y
            };
            if (pButtonLayout->scale.x <= 0)
            {
                Debug::log(
                    "Layout's width: " + std::to_string(pLayout->scale.x) + " too small "
                    "for holding info text with width: " + std::to_string(infoWidth) + " using "
                    "padding.x: "+ std::to_string(pLayout->padding.x) + " and "
                    "element gap: " + std::to_string(pLayout->elementGap) + " "
                    "(NOTE: The given layout's width is the complete width, starting from the info text to "
                    "the end of the input field! NOT THE INPUT BOX'S WIDTH!)",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            pButtonLayout->color = pLayout->color;
            pButtonLayout->hoverColor = pLayout->hoverColor;
            pButtonLayout->selectedColor = pLayout->selectedColor;
            pButtonLayout->padding = { 0, 0 };
            pButtonLayout->effectOnParentFlags = pLayout->effectOnParentFlags;
            pButtonLayout->borderColor = pLayout->borderColor;
            pButtonLayout->borderThickness = pLayout->borderThickness;
            pButtonLayout->expandElements = ExpandElements::RIGHT;

            // NOTE: textLayout.effectOnParentFlags was 0 earlier for some reason...
            // TODO: Maybe allow defining the text layout separately for the InputField?
            uint32_t buttonTextEffectOnParentFlags = EffectOnParentFlagBits::STRETCH_VERTICALLY |
                EffectOnParentFlagBits::INCREMENT_POSITION;

            Button* pButton = uiManager.createButton(
                pRootContainer,
                pButtonLayout,
                textColor,
                textHighlightColor,
                textHighlightColor, // selected color
                buttonTextEffectOnParentFlags,
                "",
                pFont,
                nullptr, //UIElement::OnClickEvent* pOnClick,
                nullptr, //UIElement::MouseEnterEvent* pOnEnter,
                nullptr //UIElement::MouseExitEvent* pOnExit
            );

            // TODO: When adding the functionality to move the cursor behind the
            // end of the string -> enable more control over the cursor color
            Layout* pCursorIndicatorLayout = uiManager.createLayout();
            pCursorIndicatorLayout->color = textColor;
            pCursorIndicatorLayout->scale = { 0, static_cast<float>(pFont->getFittingHeight()) };
            pCursorIndicatorLayout->effectOnParentFlags = EffectOnParentFlagBits::INCREMENT_POSITION;
            UIElement* pCursorIndicator = uiManager.createElement(
                pButton,
                pCursorIndicatorLayout,
                true
            );
            pCursorIndicator->setActive(false);

            InputField inputField = {
                pButton,
                pRootContainer,
                pInfoTextElement,
                pCursorIndicator
            };

            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();

            // ABSOLUTELY DISGUSTING but need to do this this way for now...
            //  -> The previous ones need to be deleted before assigning these new ones!!!
            if (pButton->_pMouseEnterEvent)
                delete pButton->_pMouseEnterEvent;
            if (pButton->_pMouseExitEvent)
                delete pButton->_pMouseExitEvent;
            pButton->_pMouseEnterEvent = new InputFieldMouseEnterEvent(pScene, inputField);
            pButton->_pMouseExitEvent = new InputFieldMouseExitEvent(pScene, inputField);

            InputManager& inputManager = pApp->getInputManager();
            inputManager.addMouseButtonEvent(new InputFieldMouseButtonEvent(pScene, inputField));
            inputManager.addCharInputEvent(new InputFieldCharInputEvent(inputField));
            inputManager.addKeyEvent(new InputFieldKeyEvent(pScene, inputField));

            return inputField;
        }


        std::string get_input_field_content(InputField inputField)
        {
            Text* pContentText = inputField.pButton->getText();
            #ifdef PLATYPUS_DEBUG
            if (!pContentText)
            {
                Debug::log(
                    "Input field's text element was nullptr!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return "";
            }
            if (!pContentText->getRenderable())
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
            return inputField.pButton->getText()->getRenderable()->text;
        }
    }
}

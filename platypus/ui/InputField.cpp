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
        void InputField::MouseEnterEvent::func(int mx, int my)
        {
            Button* pButton = _inputFieldRef._pButton;
            GUIRenderable* pButtonRenderable = pButton->getRenderable();
            if (pButtonRenderable && !_inputFieldRef.isSelected())
                pButtonRenderable->color = pButton->getLayout()->colors.hover;
        }


        void InputField::MouseExitEvent::func(int mx, int my)
        {
            Button* pButton = _inputFieldRef._pButton;
            GUIRenderable* pButtonRenderable = pButton->getRenderable();
            if (pButtonRenderable && !_inputFieldRef.isSelected())
                pButtonRenderable->color = pButton->getLayout()->colors.base;
        }


        void InputField::InputFieldMouseButtonEvent::func(
            MouseButtonName button,
            InputAction action,
            int mods
        )
        {
            std::string entityIDStr = std::to_string(_inputFieldRef._entityID);
            if (button == MouseButtonName::MOUSE_LEFT && action == InputAction::PRESS)
            {
                bool wasSelected = _inputFieldRef.isSelected();
                if (_inputFieldRef._pButton->isCursorOver())
                {
                    if (!wasSelected)
                        _inputFieldRef.setSelected(true);
                    else
                        _inputFieldRef.setSelected(false);
                }
                else
                    _inputFieldRef.setSelected(false);

                if (_inputFieldRef.isSelected())
                {
                    _inputFieldRef.setInputMode(true);
                }
                else if (wasSelected)
                {
                    _inputFieldRef.setInputMode(false);
                }
            }
        }


        void InputField::InputFieldKeyEvent::func(
            KeyName key,
            int scancode,
            InputAction action,
            int mods
        )
        {
            if (_inputFieldRef.isSelected())
            {
                Text* pText = _inputFieldRef._pButton->getText();
                std::string& str = pText->getRenderable()->text;
                if (key == KeyName::KEY_BACKSPACE && action != InputAction::RELEASE)
                {
                    util::str::pop_back_utf8(str);
                    pText->set(
                        _inputFieldRef._pButton,
                        str
                    );

                    // NOTE: WARNING!
                    //  THIS IS ABSOLUTELY DISGUSTING AND INEFFICIENT TO DO THE THING I
                    //  INITIALLY INTENDED TO DO HERE WITH THIS!
                    if (_inputFieldRef._pOnInputCharFunc)
                        (*_inputFieldRef._pOnInputCharFunc)(str, _inputFieldRef._pOnInputCharUserData);
                }
                else if (key == KeyName::KEY_ENTER)
                {
                    _inputFieldRef.setSelected(false);
                    _inputFieldRef.setInputMode(false);
                }
            }
        }


        void InputField::InputFieldCharInputEvent::func(unsigned int codepoint)
        {
            if (_inputFieldRef.isSelected())
            {
                Text* pText = _inputFieldRef._pButton->getText();
                std::string& str = pText->getRenderable()->text;
                util::str::append_utf8(codepoint, str);
                pText->set(
                    _inputFieldRef._pButton,
                    str
                );

                // NOTE: WARNING!
                //  THIS IS ABSOLUTELY DISGUSTING AND INEFFICIENT TO DO THE THING I
                //  INITIALLY INTENDED TO DO HERE WITH THIS!
                if (_inputFieldRef._pOnInputCharFunc)
                    (*_inputFieldRef._pOnInputCharFunc)(str, _inputFieldRef._pOnInputCharUserData);
            }
        }


        InputField::InputField(
            UIManager& uiManager,
            const Layout* pLayout,
            TextOverflow overflow,
            const Layout::Colors& textColors,
            const std::string& infoText,
            const Font* pFont,
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        ) :
            UIElement(
               uiManager,
               pLayout,
               false,
               NULL_ID,
               nullptr,
               false
            ),
            _pOnInputCharFunc(pOnInputCharFunc),
            _pOnInputCharUserData(pOnInputCharUserData)
        {
            _pInfoText = uiManager.createText(
                this,
                pFont,
                textColors,
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
            if (pLayout->expandElements == ui::ExpandElements::RIGHT)
            {
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
            }
            else
            {
                pButtonLayout->scale = pLayout->scale;
            }

            pButtonLayout->colors = pLayout->colors;
            pButtonLayout->padding = { 0, 0 };
            pButtonLayout->effectOnParentFlags = pLayout->effectOnParentFlags;
            pButtonLayout->borderThickness = pLayout->borderThickness;
            pButtonLayout->expandElements = ExpandElements::RIGHT;

            // NOTE: textLayout.effectOnParentFlags was 0 earlier for some reason...
            // TODO: Maybe allow defining the text layout separately for the InputField?
            uint32_t buttonTextEffectOnParentFlags = EffectOnParentFlagBits::STRETCH_VERTICALLY |
                EffectOnParentFlagBits::INCREMENT_POSITION;

            _pButton = uiManager.createButton(
                this,
                pButtonLayout,
                textColors,
                buttonTextEffectOnParentFlags,
                "",
                pFont,
                nullptr, //UIElement::OnClickEvent* pOnClick,
                nullptr, //UIElement::MouseEnterEvent* pOnEnter,
                nullptr //UIElement::MouseExitEvent* pOnExit
            );

            // TODO: When adding the functionality to move the cursor, instead of being at the
            // end of the string -> enable more control over the cursor color (+blinking etc?)
            Layout* pCursorIndicatorLayout = uiManager.createLayout();
            pCursorIndicatorLayout->colors = textColors;
            pCursorIndicatorLayout->scale = { _cursorWidth, static_cast<float>(pFont->getFittingHeight()) };
            pCursorIndicatorLayout->effectOnParentFlags = EffectOnParentFlagBits::INCREMENT_POSITION;
            _pCursorIndicator = uiManager.createElement(
                _pButton,
                pCursorIndicatorLayout,
                true
            );
            // NOTE:
            //  -> due to UIElement setActive recursion, activation and deactivation of cursor
            //  indicator needs to be handled in the overridden setActive(bool)!
            _pCursorIndicator->setActive(false);

            Application* pApp = Application::get_instance();
            Scene* pScene = pApp->getSceneManager().accessCurrentScene();

            // ABSOLUTELY DISGUSTING but need to do this this way for now...
            //  -> The previous ones need to be deleted before assigning these new ones!!!
            if (_pButton->_pMouseEnterEvent)
                delete _pButton->_pMouseEnterEvent;
            if (_pButton->_pMouseExitEvent)
                delete _pButton->_pMouseExitEvent;
            _pButton->_pMouseEnterEvent = new InputField::MouseEnterEvent(pScene, *this);
            _pButton->_pMouseExitEvent = new InputField::MouseExitEvent(pScene, *this);

            InputManager& inputManager = pApp->getInputManager();
            inputManager.addMouseButtonEvent(new InputFieldMouseButtonEvent(pScene, *this));
            inputManager.addCharInputEvent(new InputFieldCharInputEvent(*this));
            inputManager.addKeyEvent(new InputFieldKeyEvent(pScene, *this));
        }

        // Needed to override this to make the cursor indicator not show until
        // actually inputting to the field...
        // -> this otherwise does just exactly the same as UIElement's setActive
        //  -> quite dumb... TODO: Plz do something about this...
        void InputField::setActive(bool arg)
        {
            if (!arg)
                remove_from_cursor_over_layers(getAbsoluteLayer(), _entityID);

            _isCursorOver = false;
            _dragged = false;
            for (UIElement* pChild : _children)
                pChild->setActive(arg);

            Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
            pScene->setEntityActive(_entityID, arg);

            _pCursorIndicator->setActive(false);
        }

        std::string InputField::getContent()
        {
            return _pButton->getText()->getRenderable()->text;
        }

        void InputField::setContent(const std::string& text)
        {
            _pButton->getText()->set(text);
        }

        void InputField::setInputMode(bool active)
        {
            // NOTE: ISSUE! WTF!?!?!?!
            //  -> cursor is visible sometimes when the input field is NOT ACTIVE and
            //  disappears when it IS ACTIVE!!!
            // Possible reasons:
            //  *is this part of a "group" (which is fucked up anyways atm...)?
            //
            //  CONTINUE HERE!

            _pCursorIndicator->setActive(active);
            GUIRenderable* pButtonBoxRenderable = _pButton->getRenderable();
            const Layout* pBoxLayout = _pButton->getLayout();
            if (active)
            {
                Vector2f cursorScale = _pCursorIndicator->getLayout()->scale;
                // TODO: When adding functionality to select existing chars in the string
                //  -> make the cursor be the scale of the selected char!
                _pCursorIndicator->setLayoutScale({ _cursorWidth, cursorScale.y });
                pButtonBoxRenderable->color = pBoxLayout->colors.selected;
            }
            else
            {
                pButtonBoxRenderable->color = pBoxLayout->colors.base;
            }
        }
    }
}

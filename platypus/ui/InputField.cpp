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
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const Layout* pFieldLayout,
            const Layout* pFieldTextLayout,
            const Layout* pCursorIndicatorLayout,
            const std::string& infoText,
            const Font* pFont,
            void(*pOnInputCharFunc)(const std::string&, void*),
            void* pOnInputCharUserData
        ) :
            UIElement(
               uiManager,
               pParent,
               pLayout,
               false,
               NULL_ID,
               nullptr,
               nullptr,
               false
            ),
            _pOnInputCharFunc(pOnInputCharFunc),
            _pOnInputCharUserData(pOnInputCharUserData)
        {
            _pInfoText = uiManager.createText(
                this,
                pTextLayout,
                infoText,
                pFont
            );

            if (pFieldLayout->textOverflow != TextOverflow::NONE  && (pFieldLayout->scale.x == 0.0f || pFieldLayout->scale.y == 0.0f))
            {
                Debug::log(
                    "Input field layout was using text overflow, but its scale was: " + pFieldLayout->scale.toString() + ". "
                    "The scale must be larger!",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            // NOTE: textLayout.effectOnParentFlags was 0 earlier for some reason...
            // TODO: Maybe allow defining the text layout separately for the InputField?
            uint32_t buttonTextEffectOnParentFlags = EffectOnParentFlagBits::STRETCH_VERTICALLY |
                EffectOnParentFlagBits::INCREMENT_POSITION;

            _pButton = uiManager.createButton(
                this,
                pFieldLayout,
                pFieldTextLayout,
                "",
                pFont,
                nullptr, //UIElement::OnClickEvent* pOnClick,
                nullptr, //UIElement::MouseEnterEvent* pOnEnter,
                nullptr //UIElement::MouseExitEvent* pOnExit
            );

            // TODO: When adding the functionality to move the cursor, instead of being at the
            // end of the string -> enable more control over the cursor color (+blinking etc?)
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

            triggerFullTreeUpdate();
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
                // NOTE: Why the fuck is cursor indicator's scale modified here!??!!?
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

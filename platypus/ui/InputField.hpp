#pragma once

#include "Layout.hpp"
#include "Button.hpp"


namespace platypus
{
    namespace ui
    {
        class UIManager;
        class InputField : public UIElement
        {
        private:
            class MouseEnterEvent : public UIElement::MouseEnterEvent
            {
            private:
                Scene* _pScene = nullptr;
                InputField& _inputFieldRef;
            public:
                MouseEnterEvent(Scene* pScene, InputField& inputField) :
                    _pScene(pScene),
                    _inputFieldRef(inputField)
                {}
                virtual void func(int mx, int my);
            };


            class MouseExitEvent : public UIElement::MouseExitEvent
            {
            private:
                Scene* _pScene = nullptr;
                InputField& _inputFieldRef;
            public:
                MouseExitEvent(Scene* pScene, InputField& inputField) :
                    _pScene(pScene),
                    _inputFieldRef(inputField)
                {}
                virtual void func(int mx, int my);
            };


            class InputFieldMouseButtonEvent : public MouseButtonEvent
            {
            private:
                Scene* _pScene = nullptr;
                InputField& _inputFieldRef;
            public:
                InputFieldMouseButtonEvent(Scene* pScene, InputField& inputField) :
                    _pScene(pScene),
                    _inputFieldRef(inputField)
                {}
                ~InputFieldMouseButtonEvent() {}
                virtual void func(MouseButtonName button, InputAction action, int mods);
            };


            class InputFieldKeyEvent: public KeyEvent
            {
            private:
                Scene* _pScene = nullptr;
                InputField& _inputFieldRef;
            public:
                InputFieldKeyEvent(Scene* pScene, InputField& inputField) :
                    _pScene(pScene),
                    _inputFieldRef(inputField)
                {}
                ~InputFieldKeyEvent() {};
                virtual void func(KeyName key, int scancode, InputAction action, int mods);
            };


            class InputFieldCharInputEvent : public CharInputEvent
            {
            private:
                InputField& _inputFieldRef;
            public:
                InputFieldCharInputEvent(InputField& inputField) : _inputFieldRef(inputField) {}
                ~InputFieldCharInputEvent() {};
                virtual void func(unsigned int codepoint);
            };

            float _cursorWidth = 2.0f;
            Button* _pButton = nullptr;
            Text* _pInfoText = nullptr;
            UIElement* _pCursorIndicator = nullptr;

            void(*_pUserOnClickFunc)(void*);
            void* _pOnClickUserData = nullptr;

            void(*_pOnInputCharFunc)(const std::string&, void*);
            void* _pOnInputCharUserData = nullptr;

        protected:
            friend class UIManager;

            // TODO: Allow specifying the "root layout" and
            // the actual input field box layout separately!
            // NOTE: Above comment -> why? for what purpose??
            InputField(
                UIManager& uiManager,
                const Layout* pLayout,
                TextOverflow overflow,
                const Layout::Colors& textColors,
                const std::string& infoText,
                const Font* pFont,
                void(*pUserOnClickFunc)(void*) = nullptr,
                void* pOnClickUserData = nullptr,
                void(*pOnInputCharFunc)(const std::string&, void*) = nullptr,
                void* pOnInputCharUserData = nullptr
            );
            ~InputField() { }

            // Needed to override this to make the cursor indicator not show until
            // actually inputting to the field...
            // -> this otherwise does just exactly the same as UIElement's setActive
            //  -> quite dumb... TODO: Plz do something about this...
            virtual void setActive(bool arg) override;

        public:
            std::string getContent();
            void setContent(const std::string& text);

            inline Button* getButton() { return _pButton; }
            inline Text* getInfoText() { return _pInfoText; }

        private:
            void setInputMode(bool active);
        };
    }
}

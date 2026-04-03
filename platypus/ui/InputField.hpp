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

        protected:
            friend class UIManager;

            // TODO: Allow specifying the "root layout" and
            // the actual input field box layout separately!
            // NOTE: Above comment -> why? for what purpose??
            InputField(
                UIManager& uiManager,
                const Layout* pLayout,
                TextOverflow overflow,
                const Vector4f& textColor,
                const Vector4f& textHighlightColor,
                const std::string& infoText,
                const Font* pFont
            );
            ~InputField() { }

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

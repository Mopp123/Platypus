#pragma once

#include "Button.hpp"


namespace platypus
{
    namespace ui
    {
        struct InputField
        {
            Button button;
            UIElement* pContainer = nullptr;
            UIElement* pInfoText = nullptr;
            UIElement* pCursorIndicator = nullptr;
        };

        class InputFieldMouseEnterEvent : public UIElement::MouseEnterEvent
        {
        private:
            Scene* _pScene = nullptr;
            InputField _inputField;
        public:
            InputFieldMouseEnterEvent(Scene* pScene, InputField inputField) :
                _pScene(pScene),
                _inputField(inputField)
            {}
            virtual void func(int mx, int my);
        };


        class InputFieldMouseExitEvent : public UIElement::MouseExitEvent
        {
        private:
            Scene* _pScene = nullptr;
            InputField _inputField;
        public:
            InputFieldMouseExitEvent(Scene* pScene, InputField inputField) :
                _pScene(pScene),
                _inputField(inputField)
            {}
            virtual void func(int mx, int my);
        };


        class InputFieldMouseButtonEvent : public MouseButtonEvent
        {
        private:
            Scene* _pScene = nullptr;
            InputField _inputField;
        public:
            InputFieldMouseButtonEvent(Scene* pScene, InputField inputField) :
                _pScene(pScene),
                _inputField(inputField)
            {}
            ~InputFieldMouseButtonEvent() {}
            virtual void func(MouseButtonName button, InputAction action, int mods);
        };


        class InputFieldKeyEvent: public KeyEvent
        {
        private:
            Scene* _pScene = nullptr;
            InputField _inputField;
        public:
            InputFieldKeyEvent(Scene* pScene, InputField inputField) :
                _pScene(pScene),
                _inputField(inputField)
            {}
            ~InputFieldKeyEvent() {};
            virtual void func(KeyName key, int scancode, InputAction action, int mods);
        };


        class InputFieldCharInputEvent : public CharInputEvent
        {
        private:
            InputField _inputField;
        public:
            InputFieldCharInputEvent(InputField inputField) : _inputField(inputField) {}
            ~InputFieldCharInputEvent() {};
            virtual void func(unsigned int codepoint);
        };

        // TODO: Allow specifying the "root layout" and
        // the actual input field box layout separately!
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
        );

        std::string get_input_field_content(InputField inputField);
    }
}

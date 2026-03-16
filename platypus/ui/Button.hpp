#pragma once

#include "UIElement.hpp"
#include "LayoutUI.hpp"


namespace platypus
{
    namespace ui
    {
        /*
        class Button : public UIElement
        {
        private:
            Vector4f _originalColor = { 0.2f, 0.2f, 0.2f, 1.0f };
            Vector4f _highlightColor = { 0.3f, 0.3f, 0.3f, 1.0f };
            Vector4f _originalTextColor = { 0.85f, 0.85f, 0.85f, 1.0f };
            Vector4f _textHighlightColor = { 1, 1, 1, 1 };
            Vector4f _borderColor = { 0.28f, 0.28f, 0.28f, 1.0f };
            float _borderThickness = 1.0f;
            UIElement* _pText = nullptr;

        public:
            Button(
                LayoutUI& ui,
                UIElement* pParent,
                const Layout& boxLayout,
                const Layout& textLayout,
                const Vector4f& highlightColor,
                const Vector4f& textHighlightColor,
                const std::string& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick,
                UIElement::MouseEnterEvent* pOnEnter = nullptr,
                UIElement::MouseExitEvent* pOnExit = nullptr
            );
            ~Button();

            void setColors(
                const Vector4f& originalColor,
                const Vector4f& highlightColor,
                const Vector4f& selectedColor,
                const Vector4f& textOriginalColor,
                const Vector4f& textHighlightColor,
                const Vector4f& textSelectedColor
            );
        };
        */

        struct Button
        {
            UIElement* pBox = nullptr;
            UIElement* pText = nullptr;
        };


        class ButtonMouseEnterEvent : public UIElement::MouseEnterEvent
        {
        private:
            Button& _buttonRef;
        public:
            ButtonMouseEnterEvent(Button& button) : _buttonRef(button) {}
            virtual void func(int mx, int my);
        };


        class ButtonMouseExitEvent : public UIElement::MouseExitEvent
        {
        private:
            Button& _buttonRef;
        public:
            ButtonMouseExitEvent(Button& button) : _buttonRef(button) {}
            virtual void func(int mx, int my);
        };


        // TODO: Replace all above button creation with this!
        Button add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& boxLayout,
            const Layout& textLayout,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter = nullptr,
            UIElement::MouseExitEvent* pOnExit = nullptr
        );

        void reset_button(const Button& button);
    }
}

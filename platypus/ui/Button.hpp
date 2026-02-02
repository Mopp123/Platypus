#pragma once

#include "UIElement.hpp"
#include "LayoutUI.hpp"


namespace platypus
{
    namespace ui
    {
        struct Button
        {
            Vector4f originalColor;
            Vector4f highlightColor;
            Vector4f originalTextColor;
            Vector4f textHighlightColor;

            UIElement* pBox = nullptr;
            UIElement* pText = nullptr;
        };


        class ButtonMouseEnterEvent : public UIElement::MouseEnterEvent
        {
        private:
            Button _button;
        public:
            ButtonMouseEnterEvent(Button button) : _button(button) {}
            virtual void func(int mx, int my);
        };


        class ButtonMouseExitEvent : public UIElement::MouseExitEvent
        {
        private:
            Button _button;
        public:
            ButtonMouseExitEvent(Button button) : _button(button) {}
            virtual void func(int mx, int my);
        };


        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::string& text,
            const Font* pFont
        );

        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            const Vector4f& highlightColor,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const std::string& text,
            const Font* pFont,
            bool setWidthToTextWidth,
            bool setHeightToTextHeight,
            const Vector2f& scalePadding // padding if setting width and/or height to text's dimensions
        );
    }
}

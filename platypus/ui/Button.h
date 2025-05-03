#pragma once

#include "UIElement.h"
#include "LayoutUI.h"


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
            const std::wstring& text,
            const Font* pFont
        );
    }
}

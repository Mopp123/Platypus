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
            Vector4f selectedColor;
            Vector4f originalTextColor;
            Vector4f textHighlightColor;
            Vector4f textSelectedColor;

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

        class ButtonSelectEvent : public UIElement::OnClickEvent
        {
        private:
            Scene* _pScene = nullptr;
            Button _button;
            UIElement::OnClickEvent* _pUserEvent = nullptr;

        public:
            ButtonSelectEvent(Scene* pScene, Button button, UIElement::OnClickEvent* pUserEvent);
            ~ButtonSelectEvent();
            virtual void func(MouseButtonName button, InputAction action);
        };

        class ButtonDeselectEvent : public UIElement::OnClickEvent
        {
        private:
            Scene* _pScene = nullptr;
            Button _button;
        public:
            ButtonDeselectEvent(Scene* pScene, Button button, UIElement::OnClickEvent* pUserEvent);
            ~ButtonDeselectEvent();
            virtual void func(MouseButtonName button, InputAction action);
        };


        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const std::string& text,
            const Font* pFont
        );

        // NOTE: Stretches always to fit the text vertically
        //  -> stretching horizontally is optional
        UIElement* add_button_element(
            LayoutUI& ui,
            UIElement* pParent,
            const Layout& layout,
            const Vector4f& highlightColor,
            const Vector4f& selectedColor,
            const Vector4f& textColor,
            const Vector4f& textHighlightColor,
            const Vector4f& textSelectedColor,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            bool selectable,
            UIElement::OnClickEvent* pOnSelect,
            ButtonDeselectEvent* pOnDeselect
        );
    }
}

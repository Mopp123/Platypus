#pragma once

#include "UIElement.hpp"
#include "Text.hpp"


namespace platypus
{
    namespace ui
    {
        class UIManager;
        class Button : public UIElement
        {
        private:
            // NOTE: WARNING! Not sure if these MouseEnter and Exit events' names
            // are ambiguous here, since UIElement has those too...
            //  ...that should be separate namespace tho?
            class MouseEnterEvent : public UIElement::MouseEnterEvent
            {
            private:
                Button& _buttonRef;

            public:
                MouseEnterEvent(Button& button) : _buttonRef(button) { }
                virtual void func(int mx, int my);
            };

            class MouseExitEvent : public UIElement::MouseExitEvent
            {
            private:
                Button& _buttonRef;

            public:
                MouseExitEvent(Button& button) : _buttonRef(button) { }
                virtual void func(int mx, int my);
            };

            Text* _pText = nullptr;

        public:
            Button(
                UIManager& uiManager,
                const Layout* pLayout,
                const Vector4f& textColor,
                const Vector4f& textHoverColor,
                const Vector4f& textSelectedColor,
                uint32_t textEffectOnParentFlags,
                const std::string& text,
                const Font* pFont,
                UIElement::OnClickEvent* pOnClick,
                UIElement::MouseEnterEvent* pOnEnter = nullptr,
                UIElement::MouseExitEvent* pOnExit = nullptr
            );
            ~Button() { }

            void reset();
            inline Text* getText() { return _pText; }
        };



        // TODO: Replace all above button creation with this!
        /*
        Button add_button_element(
            UIManager& uiManager,
            UIElement* pParent,
            const Layout* pBoxLayout,
            const Vector4f& textColor,
            const Vector4f& textHoverColor,
            const Vector4f& textSelectedColor,
            uint32_t textEffectOnParentFlags,
            const std::string& text,
            const Font* pFont,
            UIElement::OnClickEvent* pOnClick,
            UIElement::MouseEnterEvent* pOnEnter = nullptr,
            UIElement::MouseExitEvent* pOnExit = nullptr
        );

        void reset_button(const Button* pButton);
        */
    }
}

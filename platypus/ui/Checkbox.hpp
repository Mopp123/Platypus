#pragma once

#include "Button.hpp"

namespace platypus
{
    namespace ui
    {
        class UIManager;
        class Checkbox : public UIElement
        {
        private:
            class OnSelect : public UIElement::OnClickEvent
            {
            private:
                Checkbox& _checkboxRef;
            public:
                OnSelect(Checkbox& checkboxRef) : _checkboxRef(checkboxRef) { }
                virtual ~OnSelect() {}
                virtual void func(MouseButtonName button, InputAction action);
            };

            Text* _pText = nullptr;
            Button* _pButton = nullptr;
        public:
            Checkbox(
                UIManager& uiManager,
                UIElement* pParent,
                const Layout* pLayout,
                const Layout* pTextLayout,
                const Layout* pButtonBoxLayout,
                const Layout* pButtonTextLayout,
                const std::string& text,
                const Font* pFont
            );
            ~Checkbox() { }

            void setChecked(bool arg);
            bool isChecked() const;
        };
    }
}

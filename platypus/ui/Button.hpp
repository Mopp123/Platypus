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
            Text* _pText = nullptr;

        protected:
            friend class UIManager;

            // Protected since some other UIElement may extend this
            Button(
                UIManager& uiManager,
                UIElement* pParent,
                const Layout* pLayout,
                const Layout* pTextLayout,
                const std::string& text,
                const Font* pFont,
                void(*pOnClick)(MouseButtonName, InputAction, void*),
                void* pOnClickUserData,
                void(*pOnMouseEnter)(int mx, int my, void* pUserData) = nullptr,
                void* pOnMouseEnterUserData = nullptr,
                void(*pOnMouseExit)(int mx, int my, void* pUserData) = nullptr,
                void* pOnMouseExitUserData = nullptr
            );
            ~Button() { }

        public:
            // Currently almost the same as UIElement's setActive but this also resets
            // the button's colors... dumb...
            virtual void setActive(bool arg) override;

            void reset();
            inline Text* getText() { return _pText; }

        private:
            friend void on_mouse_enter_button_default(int mx, int my, void* pUserData);
            friend void on_mouse_exit_button_default(int mx, int my, void* pUserData);
        };
    }
}

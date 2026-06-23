#include "Checkbox.hpp"
#include "UIManager.hpp"


namespace platypus
{
    namespace ui
    {
        static void on_click_checkbox(MouseButtonName button, InputAction action, void* pUserData)
        {
            if (button == MouseButtonName::MOUSE_LEFT && action == InputAction::PRESS)
            {
                Checkbox* pCheckbox = reinterpret_cast<Checkbox*>(pUserData);
                pCheckbox->setChecked(!pCheckbox->isChecked());
            }
        }

        Checkbox::Checkbox(
            UIManager& uiManager,
            UIElement* pParent,
            const Layout* pLayout,
            const Layout* pTextLayout,
            const Layout* pButtonBoxLayout,
            const Layout* pButtonTextLayout,
            const std::string& text,
            const Font* pFont
        ) :
            UIElement(
                uiManager,
                pParent,
                pLayout,
                false,
                NULL_UUID,
                nullptr,
                nullptr, // pOnClick
                nullptr, // pOnClickUserData
                false // ignore input?
            )
        {
            _pText = uiManager.createText(
                this,
                pTextLayout,
                text,
                pFont
            );

            _pButton = uiManager.createButton(
                this,
                pButtonBoxLayout,
                pButtonTextLayout,
                "",
                pFont,
                &on_click_checkbox,
                this
            );
            triggerFullTreeUpdate();
        }

        void Checkbox::setChecked(bool arg)
        {
            _pButton->setSelected(arg);
            _pButton->getText()->set(arg ? "X" : "");
        }

        bool Checkbox::isChecked() const
        {
            return _pButton->isSelected();
        }
    }
}

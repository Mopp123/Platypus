#include "Checkbox.hpp"
#include "UIManager.hpp"


namespace platypus
{
    namespace ui
    {
        void Checkbox::OnSelect::func(MouseButtonName button, InputAction action)
        {
            if (button == MouseButtonName::MOUSE_LEFT && action == InputAction::PRESS)
                _checkboxRef.setChecked(!_checkboxRef.isChecked());
        }


        Checkbox::Checkbox(
            UIManager& uiManager,
            const Layout* pLayout,
            const Layout* pButtonLayout,
            const Layout::Colors& textColors,
            const std::string& text,
            const Font* pFont
        ) :
            UIElement(
                uiManager,
                pLayout,
                false,
                NULL_ID,
                nullptr,
                false // ignore input?
            )
        {
            _pText = uiManager.createText(
                this,
                pFont,
                textColors,
                text
            );

            _pButton = uiManager.createButton(
                this,
                pButtonLayout,
                textColors,
                0, // text effect on parent
                "",
                pFont,
                new OnSelect(*this)
            );
        }

        void Checkbox::setChecked(bool arg)
        {
            if (!_pButton->isSelected())
            {
                _pButton->getText()->set("X");
                _pButton->setSelected(true);
            }
            else
            {
                _pButton->getText()->set("");
                _pButton->setSelected(false);
            }
        }

        bool Checkbox::isChecked() const
        {
            return _pButton->isSelected();
        }
    }
}

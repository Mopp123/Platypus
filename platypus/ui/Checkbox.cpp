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
                NULL_ID,
                nullptr,
                nullptr,
                false // ignore input?
            )
        {
            _pText = uiManager.createText(
                this,
                pTextLayout,
                pFont,
                text
            );

            _pButton = uiManager.createButton(
                this,
                pButtonBoxLayout,
                pButtonTextLayout,
                "",
                pFont,
                new OnSelect(*this)
            );
            triggerFullTreeUpdate();
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
